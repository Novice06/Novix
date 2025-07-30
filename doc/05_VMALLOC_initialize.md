# Virtual Aligned Memory Allocator (VMALLOC)

The kernel heap (`kmalloc`) is great for allocating small chunks of memory dynamically. However, sometimes the kernel (or future drivers and subsystems) needs **page-aligned memory allocations** — that is:

* Memory blocks of **4KB or multiples of 4KB** in size.
* Memory blocks **aligned on a 4KB boundary**.

## Implementation Overview

The design is **very similar** to the physical memory manager (see [vmalloc.c](/src/kernel/mem_manager/vmalloc.c)):

* We define a **virtual address range** dedicated to `VMALLOC`.
* We split that range into **4KB blocks**.
* We use a **bitmap** to track their usage:
   * `0` = free block
   * `1` = allocated block

## Dynamic Bitmap Allocation

Since we already have a working **kernel heap**, this allocator is simpler to implement:

* We dynamically allocate space for the bitmap using `kmalloc()`.
* This allows the system to scale with the chosen `VMALLOC` range size.
* If we ever change the VMALLOC virtual range, the code only needs minimal adjustments.

## Pre-Mapping Page Tables

Just like the **heap allocator**, we need consistency across **all future process address spaces**:

* The `VMALLOC` region must be **mapped in every page directory**.
* To guarantee this, we **pre-map all the page tables** for the VMALLOC region.
* Pages themselves are mapped only on demand when allocations occur.

This avoids page faults and ensures kernel code can always access any VMALLOC block in any process context.

## Bitmap Initialization

Once the tables are mapped:

* We initialize the bitmap to mark all blocks as **free**.
* This structure will serve as the allocation map for the entire VMALLOC region.

## Allocation and Freeing

The interface functions work similarly to the physical memory manager:

* **Allocate:**
   * Find a free sequence of 4KB blocks in the bitmap.
   * Mark them as allocated.
   * Return their **virtual address** (aligned to 4KB).

* **Free:**
   * Translate the virtual address to an index in the bitmap.
   * Mark those blocks as free.

Since VMALLOC only manages **virtual space** (not physical frames), every allocation also triggers:

* A **physical block allocation** via the physical memory manager.
* A **page mapping** operation to bind the virtual block to its physical frame(s).

## Summary

* VMALLOC provides **aligned, page-sized memory allocations** within a dedicated virtual memory region.
* The design is:
   * **Bitmap-based**, similar to the physical memory manager.
   * **Dynamic**, relying on the heap for bitmap storage.
   * **Consistent** across all address spaces due to pre-mapped tables.

This subsystem is used whenever the kernel needs **page-level memory allocations** that are:
* Physically backed.
* Properly aligned.
* Easily mappable for low-level system components.