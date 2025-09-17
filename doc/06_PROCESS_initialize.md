# Process Management System

The multitasking system is initialized by calling the `PROCESS_initialize()` function, which sets up a preemptive round-robin scheduler using the Programmable Interval Timer (PIT).

## Process Structure and States

Each process is represented by the `process_t` structure defined in [`process.h`](/src/kernel/include/proc/process.h). This structure contains all the essential information needed to manage a process: memory mappings (CR3 registers), stack pointers for both kernel and user modes, process ID, current state, and entry point.

Processes can exist in five different states:
* **DEAD** – Process has terminated and awaits cleanup
* **RUNNING** – Process is currently executing
* **READY** – Process is ready to run and waiting in the ready queue
* **BLOCKED** – Process is blocked waiting for a resource or event  
* **WAITING** – Process is waiting for a specific condition (typically used for sleeping processes)

## Scheduler Design

### Process Lists Management

The scheduler uses **linked lists** to manage processes according to their states. The main lists are:
* **Ready List** – Contains processes ready to execute, managed through `first_ready` and `last_ready` pointers
* **Dead Process List** – Contains terminated processes awaiting cleanup through the `terminated_tasks` pointer

This approach provides efficient insertion and removal operations while maintaining the order needed for round-robin scheduling.

### Scheduling Algorithm

The system implements a **round-robin preemptive scheduler** where:
* `schedule_next_process()` selects the next process from the ready queue
* `yield()` performs the actual context switch between processes
* Timer interrupt (IRQ0) triggers periodic preemptive scheduling every **5 ticks**

The scheduler supports **priority handling** where processes can be added to the ready queue with different priorities. High-priority processes are added to the front of the ready queue, while normal-priority processes are added to the end.

## Critical Process Creation

During initialization, `PROCESS_initialize()` creates two essential processes that are fundamental to the system's operation.

### Idle Process

The **idle process** is created from the current execution context that existed before multitasking initialization. This design choice is intentional because it provides a natural fallback when no other processes are ready to run.

The idle process:
* Represents the original instruction flow before multitasking
* Uses the current page directory without dynamic allocation
* Serves as the **default process** when the ready queue is empty
* Ensures the system always has something to execute

### Cleaner Process

The **cleaner process** is a dedicated kernel task responsible for cleaning up terminated processes. It:
* Runs as a kernel-mode process with its own stack
* Initially stays blocked until cleanup work is needed
* Gets woken up automatically when processes terminate
* Handles the deallocation of process resources safely

## Process Lifecycle

### Process Creation

New kernel processes are created through `PROCESS_createKernelProcess()`. The creation process involves:
1. **Memory Allocation** – Allocating the process structure and a kernel stack
2. **Stack Setup** – Preparing the stack with a return address pointing to `spawnProcess()` and space for register storage
3. **Address Space Creation** – Creating a new isolated virtual address space
4. **Registration** – Adding the process to the ready queue for scheduling

### Process Execution

Process execution begins through the `spawnProcess()` function, which:
* Unlocks the scheduler (since task switches lock it)
* Jumps to the process's actual entry point
* Provides the initial execution environment

### Process Termination

When a process terminates through `PROCESS_terminate()`:
1. The process is marked as **DEAD**
2. It's added to the terminated tasks list
3. The cleaner process is woken up if it was blocked
4. A context switch is triggered to move to another process

## Virtual Memory Management

### Address Space Creation

The `VIRTMEM_createAddressSpace()` function creates isolated virtual address spaces for new processes. The process involves:
1. **Template Copy** – Copying the current page directory as a template to preserve kernel space mappings
2. **User Space Isolation** – Clearing entries for the user space region (4MB to 3GB), creating a clean slate
3. **Recursive Mapping** – Setting up page directory self-reference for virtual access at `0xFFFFF000`
4. **Kernel Space Preservation** – Keeping kernel space mappings (3GB+) intact for system access

### Address Space Destruction

The `VIRTMEM_destroyAddressSpace()` function safely deallocates process virtual memory. This operation **must run in the cleaner process context** because it temporarily overwrites current page directory entries for access.

The destruction process:
1. **Temporary Mapping** – Maps the target address space into current context to access its pages
2. **Page Deallocation** – Systematically unmaps all pages in user space (4MB to 3GB)
3. **Page Table Cleanup** – Removes page tables for each 4MB region  
4. **Directory Cleanup** – Frees the page directory structure itself

## Time Management System

The kernel includes a comprehensive time management system that enables process sleeping capabilities and precise timing control.

### Global Time Tracking

The system maintains a **global tick counter** (`g_tickCount`) that increments with each timer interrupt, providing the foundation for all time-based operations in the kernel.

### Sleep Management

Sleeping processes are managed through a **sorted doubly-linked list** that maintains processes in chronological order of their wake-up times. This design provides several advantages:
* **Efficient Wake-Up Checks** – Only the list head needs to be checked each tick
* **Batch Processing** – Multiple processes with the same wake time are handled efficiently
* **No Unnecessary Scanning** – The system stops checking when the first non-ready process is found

When a process calls `sleep()`:
1. A sleep task structure is allocated and configured with the wake-up time
2. The structure is inserted into the sorted list at the correct chronological position
3. The process state changes to **WAITING** and a context switch occurs

### Timer Integration

The `timer()` function serves as the central timer interrupt handler and performs several critical tasks:
1. **Interrupt Acknowledgment** – Sends EOI to PIC to enable further interrupts
2. **Time Advancement** – Increments the global tick counter
3. **Sleep Management** – Checks and wakes up processes whose sleep time has elapsed
4. **Preemptive Scheduling** – Triggers context switching every 5 ticks

The **5-tick time slice** provides balanced responsiveness and efficiency while preventing process starvation.

## Synchronization and Safety

### Scheduler Locking

The system uses a **locking mechanism** to prevent race conditions during critical operations:
* `lock_scheduler()` disables interrupts and increments a counter for nested locking
* `unlock_scheduler()` decrements the counter and re-enables interrupts when safe
* All scheduler operations are protected by these locks to ensure atomicity

### Context Switching

Context switching is handled by:
* `task_switch()` – Performs low-level context switching (assembly implementation)
* `yield()` – High-level function that manages process state transitions and calls task_switch
* TSS updates for user-mode processes to ensure proper kernel stack management

## Integration Points

The process management system integrates with several other kernel subsystems:

* **Virtual Memory Manager** – For address space creation and destruction
* **Heap Allocator** – For process structure allocation
* **Timer System** – For preemptive scheduling and sleep functionality
* **Interrupt System** – Through timer interrupts and scheduler synchronization

## Synchronization Primitives - Mutex Implementation

Given that Novix is designed as a **single-processor system**, the kernel implements **mutex locks** as the primary synchronization primitive. This choice is optimal for single-CPU systems where mutexes provide sufficient protection against race conditions without the overhead of more complex synchronization mechanisms like spinlocks or atomic operations.

### Mutex Design

The mutex implementation in [`lock.c`](/src/kernel/proc/lock.c) provides **recursive locking** capabilities and maintains a **waiting queue** for blocked processes. Each mutex contains:
* A lock state and ownership information
* A recursive lock counter for multiple acquisitions by the same process
* A FIFO waiting list for processes blocked on the mutex

### Mutex Operations

#### Creation and Destruction

Mutexes are dynamically allocated through `create_mutex()` and properly cleaned up with `destroy_mutex()`. The creation process initializes all fields to safe default values, ensuring the mutex starts in an unlocked state with no waiting processes.

#### Acquiring a Mutex

When a process attempts to acquire a mutex through `acquire_mutex()`:

**If the mutex is already locked:**
* **Same Owner** – If the current process already owns the mutex, the recursive counter is incremented, allowing **recursive locking**
* **Different Owner** – The process is added to the **waiting queue** and its state changes to **WAITING**, triggering a context switch

**If the mutex is free:**
* The mutex is immediately acquired and ownership is assigned to the current process

#### Releasing a Mutex

The `release_mutex()` function handles mutex release with proper ownership validation:

1. **Ownership Check** – Only the owning process can release the mutex, preventing unauthorized releases
2. **Recursive Handling** – If the recursive count is non-zero, it's decremented without actual release
3. **Queue Management** – If processes are waiting, the **next waiting process becomes the new owner** and is unblocked
4. **Full Release** – If no processes are waiting, the mutex returns to the unlocked state

### Conditional Locking Strategy

A key optimization in the Novix mutex implementation is **conditional locking** based on multitasking state. Throughout critical sections in the kernel (such as memory allocation routines), mutexes are only acquired when multitasking is enabled:

The pattern used is:
* Check if multitasking is enabled before acquiring locks
* Perform the critical section
* Release locks only if they were acquired

This approach provides several benefits:
* **Boot Performance** – No locking overhead during kernel initialization
* **Single-Threaded Safety** – Critical sections remain protected once multitasking starts  
* **Efficiency** – Avoids unnecessary synchronization when only one execution context exists

### Integration with Process Management

The mutex system integrates seamlessly with the process management system:
* **Blocking Integration** – Uses `block_task(WAITING)` to properly block processes
* **Wake-up Integration** – Uses `unblock_task()` to wake waiting processes
* **Scheduler Coordination** – All queue operations are protected by scheduler locks

### FIFO Fairness

The mutex implementation ensures **FIFO fairness** where processes are unblocked in the order they were blocked. This prevents starvation and provides predictable behavior for processes competing for resources.

### Usage in Critical Sections

Mutexes are extensively used throughout the kernel to protect critical resources:
* **Memory Allocation** – Physical and virtual memory allocators use mutexes to prevent corruption
* **Data Structures** – Shared kernel data structures are protected by appropriate mutexes
* **Hardware Access** – Critical hardware operations are serialized through mutex protection

This mutex implementation provides robust, efficient synchronization for the single-processor Novix kernel while maintaining simplicity and optimal performance characteristics.

## Conclusion

This comprehensive process management system provides a solid foundation for multitasking in the Novix kernel, with proper resource management, synchronization, and scheduling capabilities.