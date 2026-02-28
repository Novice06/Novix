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
#include <drivers/keyboard.h>

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
    mov eax, 0xc
    mov esi, console_path
    mov ebx, 2
    int 0x80    ; opening the console device

    mov [fd_cons], edx

    mov eax, 0xc
    mov esi, keyboard_path
    mov ebx, 1
    int 0x80    ; opening the keyboard

    mov [fd_key], edx

.loop:
    mov eax, 0xd
    xor edx, edx
    mov ecx, 1
    mov ebx, [fd_key]
    mov edi, key
    int 0x80    ; tries to read from the keyboard

    or ecx, ecx
    jz .loop    ; the keyboard is empty retry

    mov bl, [key.mask]
    and bl, 01000b
    jz .loop    ; it's not a keypress (released)

    mov eax, 0x10   ; key event to ascii
    mov esi, key
    int 0x80

    mov [ascii], ebx

    mov eax, 0xe
    xor edx, edx
    mov ecx, 1
    mov ebx, [fd_cons]
    mov esi, ascii
    int 0x80            ; printing to the console

    jmp .loop

    call exit

exit:
    mov eax, 0x0
    int 0x80

    ret

key:
    .code: db 0
    .mask: db 0

fd_cons dd 0
fd_key dd 0
ascii db 0

console_path db "/dev/console"
keyboard_path db "/dev/keyboard"

*/

unsigned char user_bin[] = {
  0xb8, 0x0c, 0x00, 0x00, 0x00, 0xbe, 0x9b, 0x00, 0x40, 0x00, 0xbb, 0x02,
  0x00, 0x00, 0x00, 0xcd, 0x80, 0x89, 0x15, 0x92, 0x00, 0x40, 0x00, 0xb8,
  0x0c, 0x00, 0x00, 0x00, 0xbe, 0xa7, 0x00, 0x40, 0x00, 0xbb, 0x01, 0x00,
  0x00, 0x00, 0xcd, 0x80, 0x89, 0x15, 0x96, 0x00, 0x40, 0x00, 0xb8, 0x0d,
  0x00, 0x00, 0x00, 0x31, 0xd2, 0xb9, 0x01, 0x00, 0x00, 0x00, 0x8b, 0x1d,
  0x96, 0x00, 0x40, 0x00, 0xbf, 0x90, 0x00, 0x40, 0x00, 0xcd, 0x80, 0x09,
  0xc9, 0x74, 0xe3, 0x8a, 0x1d, 0x91, 0x00, 0x40, 0x00, 0x80, 0xe3, 0x08,
  0x74, 0xd8, 0xb8, 0x10, 0x00, 0x00, 0x00, 0xbe, 0x90, 0x00, 0x40, 0x00,
  0xcd, 0x80, 0x89, 0x1d, 0x9a, 0x00, 0x40, 0x00, 0xb8, 0x0e, 0x00, 0x00,
  0x00, 0x31, 0xd2, 0xb9, 0x01, 0x00, 0x00, 0x00, 0x8b, 0x1d, 0x92, 0x00,
  0x40, 0x00, 0xbe, 0x9a, 0x00, 0x40, 0x00, 0xcd, 0x80, 0xeb, 0xab, 0xe8,
  0x00, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x00, 0xcd, 0x80, 0xc3,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2f,
  0x64, 0x65, 0x76, 0x2f, 0x63, 0x6f, 0x6e, 0x73, 0x6f, 0x6c, 0x65, 0x2f,
  0x64, 0x65, 0x76, 0x2f, 0x6b, 0x65, 0x79, 0x62, 0x6f, 0x61, 0x72, 0x64
};
unsigned int user_bin_len = 180;


void init_process()
{
    init_device_manager();
    create_console();
    KEYBOARD_initialize();

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