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
#include <proc/time.h>
#include <hal/pic.h>
#include <hal/io.h>
#include <mem_manager/heap.h>

typedef struct sleep_tasks
{
    process_t* proc;
    uint64_t wakeTime;
    struct sleep_tasks* back;
    struct sleep_tasks* next;
}sleep_tasks_t;

sleep_tasks_t* sleeping_tasks_list;

uint64_t g_tickCount;

void add_SLEEP_process(sleep_tasks_t* proc)
{
    proc->back = NULL;
    proc->next = NULL;

    lock_scheduler();

    if(sleeping_tasks_list == NULL)
    {
        sleeping_tasks_list = proc;
        unlock_scheduler();
        return;
    }

    sleep_tasks_t* current = sleeping_tasks_list;

    while (current->next != NULL)
    {
        if(current->wakeTime > proc->wakeTime)
            break;
        
        current = current->next;
    }
    
    if(current->wakeTime > proc->wakeTime)
    {
        proc->next = current;
        proc->back = current->back;

        if(current->back != NULL)
            current->back->next = proc;
        
        current->back = proc;

        if(current == sleeping_tasks_list)
            sleeping_tasks_list = proc;
    }
    else
    {
        proc->next = current->next;
        proc->back = current;

        if(current->next != NULL)
            current->next->back = proc;

        current->next = proc;
    }

    unlock_scheduler();
}


void sleep(uint64_t ms)
{
    process_t* this_proc = PROCESS_getCurrent();

    sleep_tasks_t* new = kmalloc(sizeof(sleep_tasks_t));
    new->proc = this_proc;
    new->wakeTime = g_tickCount + ms;

    add_SLEEP_process(new);

    block_task();
}

void wakeUp_proc()
{
    while (sleeping_tasks_list != NULL)
    {
        if(sleeping_tasks_list->wakeTime > g_tickCount)
            break;

        unblock_task(sleeping_tasks_list->proc);

        sleep_tasks_t* trash = sleeping_tasks_list;

        sleeping_tasks_list = sleeping_tasks_list->next;
        if(sleeping_tasks_list != NULL)
            sleeping_tasks_list->back = NULL;

        kfree(trash);
    }
    
}

void timer(Registers* reg)
{
    PIC_sendEndOfInterrupt(0);

    g_tickCount++;

    wakeUp_proc();

    if(g_tickCount % 5 == 0)
        yield();

}