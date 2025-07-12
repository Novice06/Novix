# Novix Kernel – Project Overview

**Novix** is a simple and experimental 32-bit x86 kernel.

Novix aims to be:
- An educational project for understanding kernel development.
- A modular and maintainable base for future extensions.
- Clean code organization
- Shared utilities to avoid duplication
- Proper documentation for easier improvement and debugging

## Source Tree Overview

The project's source code is organized into the following top-level directories under `src/`:

```
src/
├── bootloader/     # Contains everything related to the boot process (Stage 1 & 2)
├── kernel/         # Core kernel logic and subsystems
├── lib/            # Shared low-level utilities (string manipulation, memory functions, etc.)
```

The `lib/` directory provides common utility functions (e.g., `memcpy`, `strlen`, etc.) that are used by both the bootloader and the kernel to prevent code duplication and to encourage modularity.

## Build System

The build process of **Novix** is coordinated using a primary `Makefile` located at the root of the project. This main Makefile orchestrates the compilation of all major components of the system in a specific order, ensuring that dependencies are built and available when needed.

### Build Phases

The build follows a **modular and hierarchical** approach, broken down into three main stages:

1. **lib/** This is the first component to be compiled. It contains low-level utility functions (such as `memcpy`, string manipulation, etc.) that are shared across both the kernel and the bootloader. By building it first, the rest of the system can link against it without redundancy.

2. **bootloader/** The second stage includes compiling both `stage1` and `stage2` of the bootloader:
   * **Stage 1** is the very first sector executed by the CPU after boot. It fits in 512 bytes and is responsible for loading the next stage.
   * **Stage 2** is a more advanced loader that can handle filesystem operations and load the kernel from the disk image.

3. **kernel/** Finally, the core kernel is compiled. It contains all main subsystems such as memory management, scheduling, drivers, etc. The kernel is built after lib and bootloader so that it can link against shared functions if needed.

### Disk Image Creation

Once all components are compiled, the system generates a **floppy disk image** (`main.img`) that contains the entire operating system. The steps are:

* A blank 1.44MB floppy image is created using `dd`.
* It is formatted as a **FAT12** filesystem.
* The boot sector (Stage 1) is written directly to the first sector.
* Stage 2 and the kernel binary are copied into the image using `mcopy` so that they can later be accessed by the bootloader.

This disk image can then be tested in an emulator like **QEMU**.

### Variable Exporting

To ensure consistency across the build system, the main Makefile also **exports environment variables** (such as paths to the compiler, assembler, and libraries). These are made available to all sub-Makefiles in `lib/`, `bootloader/`, and `kernel/`, which allows each component to be built in isolation but still follow the same configuration.

## Bootloader Overview (coming soon)

The bootloader is a key component of the Novix startup process. It is responsible for setting up the basic execution environment and loading the kernel into memory. Novix uses a two-stage bootloader (stage 1 and stage 2), designed to be simple, modular, and easy to understand.

A full technical overview of how the bootloader works including its layout, responsibilities, and interaction with the disk image will be provided in a future update to this documentation.

## Kernel Overview

The Novix kernel is designed to run in the **higher half** of the virtual address space, starting at the address `0xC0000000`. This design choice separates kernel space from user space (typically below `0xC0000000`), making the system more robust and easier to isolate.

To support this memory layout, Novix performs a careful setup of the memory mapping during early kernel initialization using two key components: a **custom linker script** (`linker.ld`) and an **assembly bootstrap file** (`entry.asm`).

### Linker Script – Relocation and Virtual Mapping

The `linker.ld` script defines how and where the kernel will be loaded in memory. Physically, the kernel is loaded at `0x00100000` (1MB), but it is **linked** to execute as if it were loaded at the virtual address `0xC0000000`. This is achieved by introducing an offset between physical and virtual memory addresses.

The linker does this by:
* Starting the `.entry` section at the physical address.
* Then **shifting all other sections** (`.text`, `.data`, `.rodata`, `.bss`, `.stack`) by a fixed offset equal to `0xC0000000 - 0x00100000`.
* Using the `AT()` directive, it ensures that sections are placed in the correct virtual memory locations but are still loaded from their correct physical positions in the binary.

This setup allows the kernel to behave as if it resides in the higher half, while being loaded by the bootloader at a physical location below 4MB.

### Initial Paging Setup – `entry.asm`

Before the CPU can execute code at higher-half addresses, **paging must be enabled** and configured to map virtual addresses to their corresponding physical addresses.

This is handled in the `entry.asm` file, which performs the following steps:

1. **Page Table Allocation**
   * Three paging structures are allocated statically at predefined physical addresses:
      * One **Page Directory Table**
      * Two **Page Tables**:
         * The first one maps the lower 4MB of memory (identity mapping).
         * The second one maps the 3GB–3GB+4MB region, which corresponds to the higher half (`0xC0000000`) mapping to `0x00100000`.

2. **Identity Mapping**
   * The first page table maps virtual addresses `0x00000000` to `0x003FFFFF` directly to the same physical addresses. This is necessary to continue executing low-level code until the jump to the higher-half address occurs.

3. **Higher-Half Mapping**
   * The second page table maps the virtual address space starting from `0xC0000000` to the physical region starting at `0x00100000` where the kernel is actually loaded. This mapping ensures that once paging is enabled, the CPU can access the kernel code and data at their expected virtual addresses.

4. **Page Directory Setup**
   * The Page Directory is filled with two entries:
      * Entry 0 → first page table (identity mapping)
      * Entry 768 → second page table (higher-half mapping) (Entry 768 corresponds to `0xC0000000` because `768 * 4MB = 3GB`)

5. **Paging Activation**
   * The page directory is loaded into `cr3`.
   * Paging is enabled by setting the PG bit (bit 31) in the `cr0` register.

6. **Stack Initialization and Higher-Half Jump**
   * Once paging is active, the stack is initialized.
   * A jump is made to a label in the `.text` section, which is now executing from the higher-half virtual address space.
   * Control is transferred to the `start` function, which is the actual C-level kernel entry point.

### Starting the Kernel

Once paging is enabled and control is transferred to the higher-half, execution continues in the `start` function, which serves as the kernel's true entry point.

At this stage, the kernel begins its initialization phase by calling the function `HAL_initialize()`. This function sets up key low-level components such as the Global Descriptor Table (GDT), the Interrupt Descriptor Table (IDT), and other hardware abstraction routines required for the kernel to run safely.

More details about what happens inside `HAL_initialize()` can be found in the dedicated document: 📄 [01_HAL_initialize.md](01_HAL_initialize.md)