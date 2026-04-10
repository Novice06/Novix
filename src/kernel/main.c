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

#include <boot_info.h>
#include <debug.h>
#include <stdint.h>
#include <utility.h>
#include <string.h>
#include <list.h>
#include <memory.h>
#include <hal/hal.h>
#include <hal/io.h>
#include <mem_manager/physmem_manager.h>
#include <mem_manager/virtmem_manager.h>
#include <mem_manager/heap.h>
#include <mem_manager/vmalloc.h>
#include <syscall/syscall.h>
#include <multitasking/scheduler.h>
#include <multitasking/process.h>
#include <multitasking/time.h>
#include <multitasking/lock.h>
#include <multitasking/ipc/message.h>
#include <multitasking/ipc/shared_memory.h>
#include <drivers/vga_text.h>
#include <drivers/device.h>
#include <drivers/console.h>
#include <drivers/keyboard.h>
#include <drivers/framebuffer.h>
#include <drivers/ata.h>

const char logo[] = 
"\
             __    __   ______   __     __  ______  __    __ \n\
            |  \\  |  \\ /      \\ |  \\   |  \\|      \\|  \\  |  \\\n\
            | $$\\ | $$|  $$$$$$\\| $$   | $$ \\$$$$$$| $$  | $$\n\
            | $$$\\| $$| $$  | $$| $$   | $$  | $$   \\$$\\/  $$\n\
            | $$$$\\ $$| $$  | $$ \\$$\\ /  $$  | $$    >$$  $$ \n\
            | $$\\$$ $$| $$  | $$  \\$$\\  $$   | $$   /  $$$$\\ \n\
            | $$ \\$$$$| $$__/ $$   \\$$ $$   _| $$_ |  $$ \\$$\\\n\
            | $$  \\$$$ \\$$    $$    \\$$$   |   $$ \\| $$  | $$\n\
             \\$$   \\$$  \\$$$$$$      \\$     \\$$$$$$ \\$$   \\$$\n\n\
";

extern uint8_t __text_start;
extern uint8_t __end;

extern uint8_t __bss_start;
extern uint8_t __bss_end;

video_info_t vidInfo;

void init_process()
{
    init_device_manager();
    create_console();
    KEYBOARD_initialize();
    FRAMEBUFFER_init(&vidInfo);
    ata_init();
    VFS_init();

    if(VFS_mount("fat32", lookup_device("sda0"), NULL) == VFS_OK)
        log_info("init_process", "sda0 mounted successfully at /");

    VFS_mkdir("/dev", 0);
    if(VFS_mount("devfs", NULL, "/dev") == VFS_OK)
        log_info("init_process", "devfs mounted successfully at /dev");

    int new_file = VFS_open("/test/file.txt", VFS_O_RDWR | VFS_O_CREAT | VFS_O_TRUNC);
    if(new_file >= 0)
    {
        char* buf = "Dans cette phrase je tente d ecrire un texte coherent et fluide.Dans cette phrase je tente d ecrire un texte coherent et fluide.Dans cette phrase je tente d ecrire un texte coherent et fluide.Dans cette phrase je tente d ecrire un texte coherent et fluide.Dans cette phrase je tente d ecrire un texte coherent et fluide.Dans cette phrase je tente d ecrire un texte coherent et fluide.Dans cette phrase je tente d ecrire un texte coherent et fluide.Dans cette phrase je tente d ecrire un texte coherent et fluide.";
        int written = VFS_write(new_file, buf, 512);

        log_debug("init", "successfully written: %d", written);

        vfs_stat_t stat;
        VFS_stat("/test/file.txt", &stat);
        log_debug("init", "file type: %d, filesize: %d", stat.type, stat.size);

        char mybuf[64];
        VFS_seek(new_file, -64, VFS_SEEK_END);
        int read = VFS_read(new_file, mybuf, 64);
        log_debug("init", "successfully read: %d", read);
        log_debug("init", "content: %s", mybuf);
    }
    VFS_close(new_file);

    struct dirent dir[5];
    int entries = VFS_getdents("/dev", dir, 5);
    for(int i = 0; i < entries; i++) printf("name: %s, type: %d\n", dir[i].name, dir[i].type);

    SYSCALL_initialize();

    // PROCESS_execve("/prog/foo.bin", NULL);
    PROCESS_execve("/doom/doomgeneric.bin", NULL);

    PROCESS_terminate();
}

void __attribute__((cdecl)) start(Boot_info* boot_info)
{
    // calculate the kernel size and memset the bss section
    uint32_t kernel_size = ((uint32_t)(&__end) - 0xc0000000 + 0x100000) - 0x100000;
    memset(&__bss_start, 0, (uint32_t)(&__bss_end) - (uint32_t)(&__bss_start));

    log_info("kernel", "the Kernel is running");

    log_info("kernel", "kernel start 0x%x, kernel end 0x%x", &__text_start, &__end);
    log_info("kernel", "kernel size %d Kb", roundUp_div(kernel_size, 1024));

    // somehow after all systems initialize Boot_info* info gets overwritten or smthing so we gonna save what we'll need first !
    memcpy(&vidInfo, &boot_info->video_info, sizeof(video_info_t));

    HAL_initialize();
    
    PHYSMEM_initialize(boot_info, kernel_size);
    VIRTMEM_initialize(kernel_size);
    HEAP_initialize();
    VMALLOC_initialize();

    List_init(kmalloc, kfree);

    SCHEDULER_initialize();
    PROCESS_createFrom(init_process);

halt:
    for(;;)
    {
        //yield();
        HLT();
    }
}