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


uint32_t taskA_id;

void taskB()
{
    for(;;)
    {
        if(send_msg(taskA_id, "Hello From Task B", strlen("Hello From Task B")) != 0)
        {
            log_debug("taskB", "what the fuck happened");
            // panic();
        }
        else
        {
            // sleep(100);
            log_err("taskB", "sent, 0x%x", PROCESS_getCurrent());
        }
    }

    PROCESS_terminate();
}

void taskC()
{
    for(;;)
    {
        if(send_msg(taskA_id, "Hey I'm The Task C", strlen("Hey I'm The Task C")) != 0)
        {
            log_debug("taskC", "what the fuck happened");
            // panic();
        }
        else
        {
            // sleep(100);
            log_warn("taskC", "sent, 0x%x", PROCESS_getCurrent());
        }
    }

    PROCESS_terminate();
}

void another_task()
{
    for(;;)
    {
        if(send_msg(taskA_id, "This is another task", strlen("This is another task")) != 0)
        {
            log_debug("another_task", "what the fuck happened");
            // panic();
        }
        else
        {
            // sleep(100);
            log_debug("another_task", "sent, 0x%x", PROCESS_getCurrent());
        }
    }

    PROCESS_terminate();
}

void taskA()
{
    uint8_t data[MAX_MESSAGE_SIZE];
    size_t size;

    taskA_id = PROCESS_getCurrent()->id;
    open_inbox();

    PROCESS_createFrom(taskB);
    PROCESS_createFrom(taskC);
    PROCESS_createFrom(another_task);

    for(;;)//int i = 5; i > 0; i--
    {
        receive_msg(data, &size);
        data[size] = 0;

        printf("Received Data : %s, size: %d\n", data, size);
        sleep(10);
    }

    PROCESS_terminate();
}

void init_process()
{
    SYSCALL_initialize();

    PROCESS_createFrom(taskA);
    // PROCESS_createFrom(taskB);
    // PROCESS_createFrom(taskC);
    // PROCESS_createFromByteArray(user_bin, user_bin_len, true);

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