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


unsigned char user_bin[] = {
0xb9, 0x05, 0x00, 0x00, 0x00, 0xb8, 0x01, 0x00, 0x00, 0x00, 0xbb, 0x2c,
0x00, 0x40, 0x00, 0xcd, 0x80, 0xb8, 0x03, 0x00, 0x00, 0x00, 0xbb, 0xf4,
0x01, 0x00, 0x00, 0xcd, 0x80, 0xe2, 0xe6, 0xe8, 0x00, 0x00, 0x00, 0x00,
0xb8, 0x00, 0x00, 0x00, 0x00, 0xcd, 0x80, 0xc3, 0x5b, 0x55, 0x53, 0x45,
0x52, 0x20, 0x41, 0x5d, 0x20, 0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73,
0x20, 0x61, 0x20, 0x75, 0x73, 0x65, 0x72, 0x6d, 0x6f, 0x64, 0x65, 0x20,
0x70, 0x72, 0x6f, 0x67, 0x72, 0x61, 0x6d, 0x20, 0x21, 0x0d, 0x0a, 0x00
};
// org 0x400000
// bits 32
// main:

//     mov ecx, 5

// .loop:
//     mov eax, 0x1
//     mov ebx, string

//     int 0x80    ; syscall
    
//     mov eax, 0x3
//     mov ebx, 500
//     int 0x80	; sleeping

//     loop .loop	; printing ecx time

//     call exit

// exit:
//     mov eax, 0x0
//     int 0x80

//     ret

// string db "[USER A] this is a usermode program !", 13, 10, 0

unsigned int user_bin_len = 84;

// void other_task()
// {
//     for(;;)
//     {
//         log_debug("other_task", "I'm another task, id: %d, eflags: 0x%x", PROCESS_getCurrent()->id, get_eflags());
//         // printf("I'm another task, id: %d, eflags: 0x%x\n", PROCESS_getCurrent()->id, get_eflags());
//     }

//     PROCESS_terminate();
// }

void taskA()
{
    for(int i = 0; i <30; i++)
    {
        printf("I'm task A, id: %d\n", PROCESS_getCurrent()->id);
    }

    PROCESS_terminate();
}

void taskB();
void cloneTaskB()
{
    for(int i = 0; i < 100; i++)
    {
        log_debug("cloneTaskB", "I'm the clone task B, id: %d, eflags: 0x%x", PROCESS_getCurrent()->id, get_eflags());
        // printf("I'm the clone task B, id: %d, eflags: 0x%x\n", PROCESS_getCurrent()->id, get_eflags());
    }

    PROCESS_createFrom(taskB);
    PROCESS_terminate();
}

void taskC();
void cloneTaskC()
{
    for(int i = 0; i < 130; i++)
    {
        log_debug("cloneTaskC", "I'm the clone task C, id: %d, eflags: 0x%x", PROCESS_getCurrent()->id, get_eflags());
        // printf("I'm the clone task B, id: %d, eflags: 0x%x\n", PROCESS_getCurrent()->id, get_eflags());
    }

    PROCESS_createFrom(taskC);
    PROCESS_terminate();
}

void taskB()
{
    for(int i = 0; i < 130; i++)
    {
        log_debug("TaskB", "I'm task B, id: %d, eflags: 0x%x", PROCESS_getCurrent()->id, get_eflags());
        // printf("I'm task B, id: %d, eflags: 0x%x\n", PROCESS_getCurrent()->id, get_eflags());
    }

    PROCESS_createFrom(cloneTaskB);
    PROCESS_terminate();
}

void taskC()
{
    for(int i = 0; i < 100; i++)
    {
        log_debug("TaskC", "I'm task C, id: %d, eflags: 0x%x", PROCESS_getCurrent()->id, get_eflags());
        // printf("I'm task B, id: %d, eflags: 0x%x\n", PROCESS_getCurrent()->id, get_eflags());
    }

    PROCESS_createFrom(cloneTaskC);
    PROCESS_terminate();
}

void init_process()
{
    SYSCALL_initialize();

    PROCESS_createFrom(taskA);
    PROCESS_createFrom(taskB);
    PROCESS_createFrom(taskC);
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

    SCHEDULER_initialize();
    PROCESS_createFrom(init_process);

    for(;;)
    {
        //yield();
        HLT();
    }
}