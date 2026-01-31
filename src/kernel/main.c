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
uint64_t shared_id;

void spy_task()
{
    uint32_t* base = shared_memory_attach(shared_id);
    log_warn("spy_task", "attached successfully at: 0x%x\n", base);

    for(;;)
    {
        sleep(1000);
        log_debug("spy_task", "spying every 1sec");
        log_info("spy", "just read: %s", base);
    }

    PROCESS_terminate();
}

void taskB()
{
    uint32_t my_id = PROCESS_getCurrent()->id;
    shared_id = shared_memory_create(1);
    printf("id shared: 0x%llx\n", shared_id);

    uint32_t* base = shared_memory_attach(shared_id);
    printf("attached successfully at: 0x%x\n", base);

    send_msg(taskA_id, &shared_id, sizeof(uint64_t));       // send the shared memory id

    PROCESS_createFrom(spy_task);   // lauching the spy process
    open_inbox();
    send_msg(taskA_id, &my_id, sizeof(uint32_t));           // sending my id so

    for(;;)
    {
        sleep(500);
        memcpy(base, "you're speaking with TaskB MotherFucker\0", 41);
        send_msg(taskA_id, NULL, 0);    // like sending a signal

        receive_msg(NULL, NULL);    // task A must issue a signal to  acknowelgde that he finished writing
        printf("Task A said: %s\n", base);
    }

    sleep(1000);
    PROCESS_terminate();
}

void taskA()
{
    uint8_t data[MAX_MESSAGE_SIZE];
    size_t size;

    taskA_id = PROCESS_getCurrent()->id;
    open_inbox();

    PROCESS_createFrom(taskB);

    receive_msg(data, &size);   // waiting for shared memory id

    uint32_t* base = shared_memory_attach(*(uint64_t*)data);
    printf("attached successfully at: 0x%x\n", base);

    receive_msg(data, &size);   // waiting for process id we're sharing data with

    for (;;)
    {
        receive_msg(NULL, NULL);    // this is like waiting for a signal
        printf("What I read: %s\n", base);

        sleep(700);
        memcpy(base, "Hello This is taskA Who is to the other side ?\0", 48);
        send_msg(*(uint32_t*)data, NULL, 0);    // like sending a signal
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

    List_init(kmalloc, kfree);

    SCHEDULER_initialize();
    PROCESS_createFrom(init_process);

    for(;;)
    {
        //yield();
        HLT();
    }
}