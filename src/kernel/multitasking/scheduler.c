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

#include <stddef.h>
#include <debug.h>
#include <hal/gdt.h>
#include <multitasking/process.h>
#include <multitasking/time.h>

void __attribute__((cdecl)) task_switch(process_t *previous, process_t *next);

process_t PROCESS_idle;
process_t* PROCESS_current;


// ready list
process_t* first_ready;
process_t* last_ready;

process_t* PROCESS_getCurrent()
{
    return PROCESS_current;
}

void add_READY_process(process_t* proc, bool high_priority)
{
    proc->state = READY;
    proc->next = NULL;

    if(first_ready == NULL)
    {
        first_ready = proc;
        last_ready = proc;

        return;
    }

    if(high_priority)
    {
        proc->next = first_ready;
        first_ready = proc;

        return;
    }

    last_ready->next = proc;
    last_ready = proc;
}

process_t* schedule_next_process()
{
    // the caller is responsible for making sure the scheduler is locked before using this fucntion

    if(first_ready == NULL)
    {
        if(PROCESS_current->state != RUNNING)
            return &PROCESS_idle;
        else
            return PROCESS_current;
    }

    process_t* ret = first_ready;

    first_ready = first_ready->next;

    return ret;
}

void yield()
{
    process_t* prev = PROCESS_current;
    process_t* next = schedule_next_process();

    if(next != prev)
    {
        if(prev->state == RUNNING && prev != &PROCESS_idle)
            add_READY_process(prev, false);

        if(next->usermode)
            TSS_setKernelStack((uint32_t)next->esp0 + 0x1000);

        PROCESS_current = next;
        PROCESS_current->state = RUNNING;

        task_switch(prev, next);
    }
}

void SCHEDULER_initialize()
{
    PROCESS_initialize(&PROCESS_idle);

    PROCESS_current = &PROCESS_idle;
    PROCESS_current->state = RUNNING;

    IRQ_registerNewHandler(0, timer);   // pit interrupt
}