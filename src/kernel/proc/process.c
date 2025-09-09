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

#include <debug.h>
#include <hal/io.h>
#include <proc/process.h>
#include <proc/time.h>
#include <stddef.h>
#include <hal/gdt.h>
#include <mem_manager/virtmem_manager.h>
#include <mem_manager/heap.h>
#include <mem_manager/vmalloc.h>

void __attribute__((cdecl)) task_switch(process_t *previous, process_t *next);

process_t* PROCESS_current;
process_t* PROCESS_idle;
process_t* PROCESS_cleaner;

// ready list
process_t* first_ready;
process_t* last_ready;

// dead process list
process_t* terminated_tasks;

int IRQ_disable_counter = 0;

void lock_scheduler()
{
    disableInterrupts();
    IRQ_disable_counter++;
}

void unlock_scheduler()
{
    if(IRQ_disable_counter > 0)
        IRQ_disable_counter--;

    if(IRQ_disable_counter == 0)
        enableInterrupts();
}

void add_READY_process(process_t* proc, bool high_priority)
{
    lock_scheduler();

    proc->state = READY;
    proc->next = NULL;

    if(first_ready == NULL)
    {
        first_ready = proc;
        last_ready = proc;
    }
    else
    {
        if(high_priority)
        {
            proc->next = first_ready;
            first_ready = proc;
        }
        else
        {
            last_ready->next = proc;
            last_ready = proc;
        }
        
    }   
    
    unlock_scheduler();
}

void block_task()
{
    lock_scheduler();
    PROCESS_current->state = BLOCKED;
    unlock_scheduler();
    
    yield();
}

void unblock_task(process_t* proc)
{
    lock_scheduler();

    add_READY_process(proc, true);

    unlock_scheduler();
}

process_t* schedule_next_process()
{
    // the caller is responsible for making sure the scheduler is locked before using this fucntion

    if(first_ready == NULL)
    {
        if(PROCESS_current->state != RUNNING)
            return PROCESS_idle;
        else
            return PROCESS_current;
    }

    process_t* ret = first_ready;

    first_ready = first_ready->next;
    
    if(first_ready == NULL)
        last_ready =  NULL;

    return ret;
}

void yield()
{
    lock_scheduler();

    process_t* prev = PROCESS_current;
    process_t* next = schedule_next_process();

    if(next != prev)
    {
        if(prev->state == RUNNING && prev != PROCESS_idle)
            add_READY_process(prev, false);

        if(next->usermode)
            TSS_setKernelStack((uint32_t)next->esp0);

        PROCESS_current = next;
        PROCESS_current->state = RUNNING;

        task_switch(prev, next);
    }

    unlock_scheduler();
}

void spawnProcess()
{
    void (*entryPoint)();

    entryPoint = PROCESS_current->entryPoint;

    unlock_scheduler(); // unlocks beacause a task switch locks the scheduler
    entryPoint();
}

process_t* PROCESS_getCurrent()
{
    return PROCESS_current;
}

void cleaner_task()
{
    while (true)
    {
        if(terminated_tasks != NULL)
        {
            lock_scheduler();

            process_t* trash = terminated_tasks;
            terminated_tasks = terminated_tasks->next;

            log_debug("cleaner", "cleaning 0x%x", trash);

            unlock_scheduler();

            // TODO: add support to clean usermode process

            vfree(trash->esp0);
            kfree(trash);

            continue;
        }

        block_task();
        yield();
    }
    
}

void PROCESS_initializeMultiTasking()
{
    // create the first process which is the idle process
    PROCESS_idle = kmalloc(sizeof(process_t));

    PROCESS_idle->esp0 = NULL;  // it's a kernel mode task.
    PROCESS_idle->esp = NULL;   // this will be filled automatically when a context switch occurs

    PROCESS_idle->cr3 = getPDBR();
    PROCESS_idle->usermode = false;
    PROCESS_idle->entryPoint = NULL;

    PROCESS_idle->next = NULL;

    PROCESS_current= PROCESS_idle;
    PROCESS_current->state = RUNNING;

    PROCESS_cleaner = kmalloc(sizeof(process_t));

    PROCESS_cleaner->esp0 = vmalloc(1);
    PROCESS_cleaner->esp = PROCESS_cleaner->esp0 + 0x1000 - 4;

    *(uint32_t*)PROCESS_cleaner->esp = (uint32_t)spawnProcess; // return address after task_switch

    PROCESS_cleaner->esp -= (4 * 5);   // pushed register
    *(uint32_t*)PROCESS_cleaner->esp = 0x202;       // default eflags for the new process

    PROCESS_cleaner->cr3 = getPDBR();
    PROCESS_cleaner->usermode = false;
    PROCESS_cleaner->entryPoint = cleaner_task;
    PROCESS_cleaner->state = BLOCKED;

    PROCESS_cleaner->next = NULL;
    IRQ_registerNewHandler(0, timer); // preemptive multi tasking
}

void PROCESS_createKernelProcess(void* task)
{
    process_t* proc = kmalloc(sizeof(process_t));

    // here I'm using the esp0 field to store the starting address of the heap just to make
    // dealocation easier but otherwise this field must be null since it's a kernel process
    proc->esp0 = vmalloc(1);

    proc->esp = proc->esp0 + 0x1000 - 4;

    *(uint32_t*)proc->esp = (uint32_t)spawnProcess; // return address after task_switch

    proc->esp -= (4 * 5);   // pushed register
    *(uint32_t*)proc->esp = 0x202;       // default eflags for the new process

    proc->cr3 = getPDBR();
    proc->usermode = false;
    proc->entryPoint = task;

    proc->next = NULL;

    add_READY_process(proc, false);
}

void PROCESS_terminate()
{
    lock_scheduler();

    PROCESS_current->state = DEAD;

    PROCESS_current->next = terminated_tasks;
    terminated_tasks = PROCESS_current;

    if(PROCESS_cleaner->state == BLOCKED)
        unblock_task(PROCESS_cleaner);

    unlock_scheduler();
    yield();
}