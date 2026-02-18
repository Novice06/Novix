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
#include <drivers/device.h>
#include <drivers/console.h>

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

/*

org 0x400000
bits 32
main:

    mov eax, 0xc    ; open
    mov esi, console_path
    mov ebx, 3
    int 0x80

    mov ecx, 0
    cmp edx, ecx
    jl exit     ; failed to open

    mov eax, 0xe    ; write
    mov edx, 0
    mov ecx, 30     ; the size of the buffer
    mov ebx, edx    ; the descriptor
    mov esi, proof  ; buffer
    int 0x80

    call exit

exit:
    mov eax, 0x0
    int 0x80

    ret

proof db "User writing to the console", 0, 13, 10
console_path db "/dev/console"

*/

unsigned char user_bin[] = {
  0xb8, 0x0c, 0x00, 0x00, 0x00, 0xbe, 0x5d, 0x00, 0x40, 0x00, 0xbb, 0x03,
  0x00, 0x00, 0x00, 0xcd, 0x80, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x39, 0xca,
  0x7c, 0x1d, 0xb8, 0x0e, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00, 0x00,
  0xb9, 0x1e, 0x00, 0x00, 0x00, 0x89, 0xd3, 0xbe, 0x3f, 0x00, 0x40, 0x00,
  0xcd, 0x80, 0xe8, 0x00, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x00,
  0xcd, 0x80, 0xc3, 0x55, 0x73, 0x65, 0x72, 0x20, 0x77, 0x72, 0x69, 0x74,
  0x69, 0x6e, 0x67, 0x20, 0x74, 0x6f, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63,
  0x6f, 0x6e, 0x73, 0x6f, 0x6c, 0x65, 0x00, 0x0d, 0x0a, 0x2f, 0x64, 0x65,
  0x76, 0x2f, 0x63, 0x6f, 0x6e, 0x73, 0x6f, 0x6c, 0x65
};
unsigned int user_bin_len = 105;


void init_process()
{
    init_device_manager();
    create_console();

    VFS_init();
    if(VFS_mount("ramfs", NULL) == VFS_OK)
        log_info("init_process", "ramfs mounted successfully at /");

    // we should make the directory first!
    if(VFS_mount("devfs", "/dev") == VFS_OK)
        log_info("init_process", "devfs mounted successfully at /dev");

    SYSCALL_initialize();

    PROCESS_createFromByteArray(user_bin, user_bin_len, true);

    PROCESS_terminate();
}

void __attribute__((cdecl)) start(Boot_info* info)
{
    // calculate the kernel size and memset the bss section
    uint32_t kernel_size = ((uint32_t)(&__end) - 0xc0000000 + 0x100000) - 0x100000;
    memset(&__bss_start, 0, (&__bss_end) - (&__bss_start));

    log_info("kernel", "the Kernel is running");

    log_info("kernel", "kernel start 0x%x, kernel end 0x%x", &__text_start, &__end);
    log_info("kernel", "kernel size %d Kb", roundUp_div(kernel_size, 1024));

    puts(logo);

    HAL_initialize();
    
    PHYSMEM_initialize(info, kernel_size);
    VIRTMEM_initialize(kernel_size);
    HEAP_initialize();
    VMALLOC_initialize();

    List_init(kmalloc, kfree);

    SCHEDULER_initialize();
    PROCESS_createFrom(init_process);

    for(;;)
    {
        //yield();
        HLT();
    }
}