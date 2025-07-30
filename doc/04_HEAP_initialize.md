# Kernel Heap Initialization

Now that we have a **working physical memory manager** and a **virtual memory manager**, the next step is to allow the OS to **allocate small memory blocks** efficiently.

## Why Not Just Allocate Pages?

We could have simply reserved a virtual memory range — for example:

```
0xD0000000 → 0xD0400000  (4MB reserved for the heap)
```

Then, every time we need to allocate memory (even just 4 bytes), we could map a whole page.

But that's incredibly **wasteful**.

We needed a **robust, flexible, and memory-efficient allocator** for the kernel (see [heap.c](/src/kernel/mem_manager/heap.c))

## Design Inspiration

1. **Imitating `sbrk()` (from Linux)** Like in Linux, the kernel heap behaves like a **growing memory region**. We track the current "break" (`brk`) — the end of the heap — and extend it when needed.

2. I implemented a **doubly-linked list** to track allocated blocks Inspired by this [tutorial](/doc/arjunsreedharan_org_post_148675821737_memory_allocators_101_write_a_simple_memory.pdf), then improved it with an **ordered array** (also from another [tutorial](/doc/heap_tuto.pdf)) to track free blocks efficiently using a **best-fit** strategy.

## Initialization Overview

### Step 1: Set Up Key Global Variables

* `brk`: Similar to the Unix `sbrk`, it marks the end of currently allocated heap space.
* `lastHeapAllocatedPage`: Keeps track of the last fully mapped page of the heap.

This is important because if `brk` tries to cross this boundary, we'll need to **map a new page**.

### Step 2: Pre-map Page Tables

Before allocating any memory, we **pre-map the page tables** (not the pages) for the **entire heap range**.

Why?

In future, when we implement process address spaces, we'll map the **entire kernel space** into **every process**. But the heap is volatile and grows over time — to make it consistent across all address spaces, the **page tables** must already exist.

This ensures that **no matter the process**, the kernel heap space will be safely and consistently addressable.

### Step 3: Reserve Space for the Ordered Array

To implement the **best-fit allocator**, we need an **ordered array** to track all free blocks.

* This array is **static** (fixed-size), so we need to reserve space for it.
* I chose to place it at the **very beginning** of the heap range.
* That means the actual usable heap memory starts at:

```
HEAP_START_ADDR + 0x1000  // skip the first 4KB for the ordered array
```

We **map the memory** for the ordered array and initialize it first.

Then we **map one page** for the heap itself and update `lastHeapAllocatedPage`.

## Mimicking `sbrk()` Logic

The core of the allocator revolves around an imitation of `sbrk()`:

* If `brk + size` is still within the current page → no problem, just return a pointer.
* If it exceeds the current page → map new pages and update `lastHeapAllocatedPage`.
* Free blocks are inserted back into the ordered array for reuse.

This logic lets the kernel **grow its heap dynamically** and reuse freed blocks efficiently, all while avoiding page waste.

## Summary

The kernel heap allocator is a hybrid design:

* Inspired by `sbrk()`, for dynamic memory growth.
* Optimized with an **ordered array** to reduce fragmentation via **best-fit** allocation.
* Designed to work **safely within virtual memory**, with future **multitasking support** in mind.

This forms the basis for all future `kmalloc()` / `kfree()`-style memory management in the kernel.