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
#include <proc/lock.h>
#include <mem_manager/heap.h>

mutex_t* create_mutex()
{
    mutex_t* mut = kmalloc(sizeof(mutex_t));

    mut->locked = false;
    mut->locked_count = 0;
    mut->owner = NULL;
    mut->first_waiting_list = NULL;
    mut->last_waiting_list = NULL;

    return mut;
}

void destroy_mutex(mutex_t* mut)
{
    kfree(mut);
}

void acquire_mutex(mutex_t* mut)
{
    process_t* current_process = PROCESS_getCurrent();

    if(mut->locked)
    {
        if(mut->owner == current_process)
        {
            mut->locked_count++;    // it's okay we can acquire a mutex multiple time
            return; 
        }
        
        lock_scheduler();
        current_process->next = NULL;

        if(mut->first_waiting_list == NULL)
        {
            mut->first_waiting_list = current_process;
            mut->last_waiting_list = current_process;
        }
        else
        {
            mut->last_waiting_list->next = current_process;
            mut->last_waiting_list = current_process;
        }

        block_task();
        unlock_scheduler();
        yield();
    }
    else
    {
        mut->locked = true;
        mut->owner = current_process;
    }
}

void release_mutex(mutex_t* mut)
{
    process_t* current_process = PROCESS_getCurrent();

    if(mut->owner != current_process)
    {
        log_err("mutex", "Process 0x%x tried to release mutex it doesn't own!", current_process->entryPoint);
        return;
    }

    if(mut->locked_count != 0)
    {
        mut->locked_count--;
        return;
    }

    if(mut->first_waiting_list != NULL)
    {
        lock_scheduler();

        process_t* released = mut->first_waiting_list;

        mut->first_waiting_list = mut->first_waiting_list->next;

        mut->owner = released;  // new owner

        unblock_task(released);
        unlock_scheduler();
    }
    else
    {
        mut->locked = false;
        mut->owner = NULL;
    }
}
