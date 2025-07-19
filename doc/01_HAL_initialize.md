# HAL_initialize

The `HAL_initialize()` function is responsible for setting up key low-level subsystems required for the kernel to run safely. It performs the following steps:

## GDT and IDT Initialization

The GDT and IDT are initialized in the files [`gdt.c`](/src/kernel/hal/gdt.c) and [`idt.c`](/src/kernel/hal/idt.c), respectively. Each implementation simply sets up the structures and submits them to the CPU using the appropriate instructions (`lgdt` and `lidt`). The code is straightforward and self-explanatory, and the logic is kept as simple and direct as possible.

## Interrupt Service Routines (ISRs)

Now that the IDT is initialized, the next step is to handle interrupts. The ISR setup in Novix is inspired by **Nanobyte's approach**, where all ISRs converge to a single, common handler.

In [`isr.asm`](/src/kernel/hal/isr.asm), there is a label called `isr_common`, which acts as the unified entry point for all interrupts. This label pushes the current CPU state onto the stack and then passes a pointer to that state into a C handler function. This pointer is interpreted as a `Registers` structure (defined in [`isr.h`](/src/kernel/include/hal/isr.h)).

To make this system work, we need one function per ISR that simply jumps to `isr_common`. To avoid writing 256 ISR stubs manually, two macros are used to generate them:
* One macro for ISRs **without an error code**
* Another for ISRs **with an error code**

For ISRs that do not naturally push an error code, a dummy value is pushed to ensure the stack remains consistent for all ISRs. This makes the `Registers` structure layout uniform and compatible across all interrupts.

Instead of manually writing all these stubs, a script named [`isr_generator.sh`](/src/kernel/hal/isr_generator.sh) is used to generate two files at build time:
* `isrs_gen.inc` – contains all the ISR stub functions generated using the macros, handling both error-code and non-error-code variants.
* `isrs_gen.c` – contains the function `ISR_initializeGates()`, which registers each ISR stub into the IDT using `IDT_setGate()` from [`idt.c`](/src/kernel/hal/idt.c).

This design ensures that all ISRs ultimately route to a **single handler function** while maintaining flexibility and maintainability.

### Dynamic ISR Handler Registration

A key mechanism in this design is the ability to register custom ISR handlers.

In [`isr.c`](/src/kernel/hal/isr.c), there is a global array of function pointers: `g_ISR_handlers`. In the central handler function `ISR_handler()`, if a handler exists for the incoming interrupt in this array, it is called. Otherwise, the system falls back to a default action, such as displaying a message or triggering a kernel panic.

New handlers can be registered using the function `ISR_registerNewHandler()`, which simply associates a handler function with a given interrupt number.

## IRQs – Hardware Interrupts

After setting up the ISRs, we move on to handling **hardware interrupts (IRQs)**. The mechanism is similar to that of ISRs: all IRQs are routed to a **common handler**, which then dispatches them using a function pointer array for individual processing.

### PIC – Programmable Interrupt Controller

To receive hardware IRQs, we must first **configure the PIC 8259A**. This is done in the function `IRQ_initialize()` (in [`irq.c`](/src/kernel/hal/irq.c)), which begins by programming the two PIC chips (master and slave).

The driver implementation is in [`pic.c`](/src/kernel/hal/pic.c). It provides functions for:
* Masking/unmasking IRQ lines
* Reading status registers
* Sending end-of-interrupt signals (EOI)
* General configuration utilities

The PIC requires **remapping** because the first 32 interrupt vectors (`0x00–0x1F`) are reserved for CPU exceptions. We remap the master PIC to start from `0x20` (vector 32) to avoid conflicts. The configuration sets both PICs into **edge-triggered mode**, and other required settings for correct operation on x86.

🔹 **Important:** In this design, **each IRQ handler is responsible for sending the EOI** (End Of Interrupt) to the PIC once the interrupt is processed.

### PIT – Programmable Interval Timer

Once the PIC is configured, we initialize the **PIT** (in [`pit.c`](/src/kernel/hal/pit.c)) to generate regular timer interrupts every **1 millisecond**. This timer is essential for basic multitasking and timekeeping in the kernel.

### IRQ Setup Summary

After configuring the PIC and PIT, we proceed to:
* Register the **entire IRQ vector range (32 to 47)** to the **common IRQ handler**, just like with ISRs.
* Install a **dedicated handler for IRQ0** (the PIT timer), which avoids printing messages every 1ms unnecessarily.

Finally, after all this setup, we **enable interrupts** for the **first time in the kernel**.