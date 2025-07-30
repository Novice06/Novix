# Physical Memory Initialization

Physical memory management (in [physmem_manager.c](/src/kernel/mem_manager/physmem_manager.c)) is one of the **most critical parts of the kernel**, as it ensures we never overwrite regions already in use, like the kernel image or hardware-reserved memory zones. A mistake in managing physical memory can lead to corruption, crashes, or undefined behavior.

In this implementation, we provide a mechanism to **dynamically allocate fixed-size physical memory blocks**. Each block is **4KB**, aligning with the paging granularity of the system (4KB pages).

To manage this, the entire available physical memory is conceptually **divided into 4KB blocks**, and their usage is tracked using a **bitmap**, where:
* *`0`* represents a **free** block,
* *`1`* represents a **used** block.

This bitmap is just an array of bits (not bytes), with **one bit per 4KB block** in physical memory.

## Memory Map and Initialization Inputs

The `PHYSMEM_initialize()` function needs two critical inputs:
* The **memory map** provided by the bootloader.
* The **kernel's size in memory**, so we avoid allocating memory already used by it.

These inputs are encapsulated in a `Boot_Info` structure (see [boot_info.h](/src/lib/boot_info.h)), which contains:
* A pointer to an array of memory regions (base address, length, type).
* A `memoryBlockCount` indicating how many entries are present (max 256).

## Allocating the Bitmap Dynamically (No Allocator Yet!)

One of the **most interesting challenges** in physical memory management is allocating memory for the bitmap itself *without* having a memory allocator yet.

The approach taken is:
1. Scan the bootloader-provided memory map.
2. Find a **large enough free memory region** to hold the bitmap.
3. Allocate the beginning of that region to store the bitmap.
4. Add a **new memory map entry** that marks the bitmap region as reserved.
   * Note: this leads to **overlapping regions** in the memory map, as the original free block is still listed.

This dynamic bitmap allocation allows the memory manager to scale with systems of various memory sizes without relying on a fixed-size bitmap.

## Initializing the Memory Map for 4KB Blocks

Once basic structures are in place, the next step is to create a **normalized version of the memory map**, stored in `g_memory4KbEntries`. This array:
* Is globally accessible.
* Starts as a clone of the bootloader memory map.

Then, we process it with `PHYSMEM_memoryMapToBlock()` to trasform it to a 4kb aligned block memory map

## Mapping into the Bitmap

This is a **very delicate stage**. Here's the core idea:
* The bitmap is **initially set to "all used"**.
* Then we **mark the free blocks**.
* Finally, we **mark the reserved blocks** again.

Why this order?

Because of **overlapping memory map entries**, like the case of the dynamically allocated bitmap:
* A free memory region may contain a reserved subregion (e.g., the bitmap).
* By marking free blocks first, we "open up" the space.
* By marking used blocks *after*, we "shrink" the free zones accordingly — **automatically resolving overlaps**.

This strategy avoids the need to perform complex memory map merging or splitting logic.

Finally, the function counts the number of **free and used blocks** for bookkeeping.

## Allocation and Freeing

After initialization, memory allocation becomes straightforward:
* To **allocate a block**: find a `0` in the bitmap, flip it to `1`, and return its physical address.
* To **free a block**: compute the index from the physical address, and flip the corresponding bit back to `0`.

The address ↔︎ index conversion uses simple 4KB multiplication and division.