# Virtual Memory Initialization

Now that we have a working physical memory manager, you might think we could just use it for all memory allocation in the OS. However, that **won't work in practice**. Why?

Because ever since we **enabled paging** early in the boot process, we have only **identity-mapped** the **first 4MB** of physical memory. That means we can *only safely access* physical memory that lies **below 0x00400000 (4MB)** using normal pointers.

This identity mapping was great at first — it even allowed us to allocate and access the bitmap of the physical memory manager without any extra effort. But now, it's time to do things properly.

## Why We Need Virtual Memory

Relying solely on identity-mapped memory is **very limiting**:
* We can't access physical memory beyond 4MB safely.
* We have no protection or isolation between components.
* Memory structures like page directories and page tables were initially allocated at **arbitrary static addresses**, not tracked by the physical memory manager.

This is dangerous — we risk overwriting those structures later since they were never registered as used memory!

To solve this, we introduce a **virtual memory manager** (in [virtmem_manager.c](/src/kernel/mem_manager/virtmem_manager.c)) that:
* Dynamically allocates page tables and directories using the physical memory manager.
* Properly maps virtual addresses to physical memory.
* Adds a critical feature: **recursive page directory mapping**.

## What Initialization Does

The `VIRTMEM_initialize()` function revisits the early paging setup and **does it right** this time.

### 1. Rebuild Paging Structures Properly

Instead of static allocations, we:
* Allocate a new **page directory** and **two page tables** using `PHYSMEM_AllocBlock()`.
* These allocations are now tracked and safe.

We then:
* Map the first 4MB again (identity-mapped) using the two new tables.
* Use flags like `present`, `writable`, and `user/kernel` where needed.

### 2. Add Recursive Page Mapping

The key feature of this setup is **recursive mapping**:

We map the **last entry** (entry 1023) of the page directory to **point to the page directory itself**.

This gives us a powerful trick:
* The page tables become accessible at virtual addresses like `0xFFC00000` and above.
* We can dynamically access and manipulate the paging structures **from virtual space**.
* It eliminates the need to identity-map new tables temporarily.

## After Initialization

Once the virtual memory manager is set up:
* We can write generic functions to **map**, **unmap**, and **translate** virtual addresses.
* Memory allocations beyond 4MB become safe and normal.
* All kernel components can use high-level virtual memory APIs instead of fiddling with physical addresses.
