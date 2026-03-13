#pragma once

// 0x00000000 - 0x000003FF - interrupt vector table
// 0x00000400 - 0x000004FF - BIOS data area


// 0x00000500 - 0x0000F500 - stage2

// 0x00020000 - 0x00026000 - FAT driver
#define MEMORY_FAT_ADDR     ((void*)0x20000)
#define MEMORY_ROOTDIR_ADDR     ((void*)0x23000)
#define MEMORY_FATBUFFER_ADDR     ((void*)0x36000)

// 0x00040000 - 0x00040100 - MEMORY information
#define MEMORY_MAP_ADDR     ((void*)0x40000)

// 0x00040200 - 0x00040400 - BOOT information

#define MEMORY_BOOTINFO_ADDR     ((void*)0x40200)

// 0x00040400 - 0x00080000 - free

// 0x00080000 - 0x0009FFFF - Extended BIOS data area
// 0x000A0000 - 0x000C7FFF - Video
// 0x000C8000 - 0x000FFFFF - BIOS