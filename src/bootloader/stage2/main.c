/*
 * Copyright (C) 2025,  Novice
 *
 * This file is part of the Novix software.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/



#include <stdint.h>
#include <memory.h>
#include "stdio.h"
#include "disk.h"
#include "fat32.h"
#include "x86.h"
#include "memdefs.h"
#include "memdetect.h"
#include "vbe.h"

void* Kernel_addr = (void*)0x100000;
typedef void __attribute__((cdecl)) (*KernelStart)(Boot_info* info);

Boot_info* g_info = MEMORY_BOOTINFO_ADDR;
DISK disk;

typedef struct
{
    uint8_t boot_flag;
    uint8_t start_head;
    uint16_t starting_sector_and_starting_cylinder;
    uint8_t system_id;
    uint8_t ending_head;
    uint16_t ending_sector_and_starting_cylinder;
    uint32_t relative_sector;
    uint32_t total_sectors;
} __attribute__((packed)) partition_entry_t;

struct
{
    uint8_t noise[512 - (sizeof(partition_entry_t) * 4) - 2];
    partition_entry_t entries[4];
    uint16_t boot_signature;
}mbr;

partition_entry_t* boot_partition;
void read_onPartition(void* buffer, uint32_t lba, uint32_t sector_num)
{
    lba += boot_partition->relative_sector;
    DISK_ReadSectors(&disk, lba, sector_num, buffer);
}

void __attribute__((cdecl)) start(uint16_t bootDrive)
{
    clr();
    printf("booted from: 0x%x\n", bootDrive);

    // if(!init_graphics(1440, 900, &g_info->video_info)) 
    // {
    //     printf("graphics must had failed or something !!\n");
    //     goto end;
    // }
    
    if (!DISK_Initialize(&disk, bootDrive))
    {
        printf("Disk init error\r\n");
        goto end;
    }

    
    DISK_ReadSectors(&disk, 0, 1, &mbr);

    // searching for the bootable partition WE MUST find it otherwise it doesn't make any sense
    for(int i = 0; i < 4; i++)
    {
        if(mbr.entries[i].boot_flag & 0x80)
        {
            boot_partition = &mbr.entries[i];
            break;
        }
    }

    if(!boot_partition)
    {
        debugf("no boot partition found!!\n");
        goto end;
    }

    // printf("partition start at: %d, total sectors: %d\n", boot_partition->relative_sector, boot_partition->total_sectors);
    
    fat32_init(read_onPartition);

    file_t* kernel = fat32_open("/kernel.bin");
    if(!kernel) goto end;

    printf("kernel found proof: name: %s, size: %d\n", kernel->entry->filename, kernel->entry->fileSize);
    // goto end;

    int read = fat32_read(kernel, Kernel_addr, kernel->entry->fileSize);

    if(read != kernel->entry->fileSize) goto end;

    printf("memory map\n");
    memoryDetect(g_info);
    g_info->bootDrive = bootDrive;
    // for(int i = 0; i < g_info->memoryBlockCount; i++)
    //     printf("base: 0x%llx, length: 0x%llx, type: %d, acpi: %d\n", g_info->memoryBlockEntries[i].base, g_info->memoryBlockEntries[i].length, g_info->memoryBlockEntries[i].type, g_info->memoryBlockEntries[i].acpi);

    // goto end;

    // printf("jump to the kernel but before:\n\n");
    // print_buffer("kernel: ", (void*)Kernel_addr + (16 * 512), 512);
    goto end;

    // execute kernel
    KernelStart kernelStart = (KernelStart)Kernel_addr;
    kernelStart(g_info);

end:
    debugf("Unable to load the Kernel !!\n");
    for (;;);
}
