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
#include <hal/io.h>
#include <multitasking/scheduler.h>
#include <multitasking/lock.h>
#include <mem_manager/heap.h>

void lock_scheduler(uint32_t *saved_Eflags)
{
    *saved_Eflags = get_eflags();     // we need to save the current state so we can re-enable interrupt in the right places
    
    disableInterrupts();
}

void unlock_scheduler(uint32_t *saved_Eflags)
{
    // if before locking the scheduler we were is a state where interrupt were enabled
    if(*saved_Eflags & (1 << 9))
        enableInterrupts();
}

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

    uint32_t eFlags;
    lock_scheduler(&eFlags);

    if(mut->locked)
    {
        if(mut->owner == current_process && mut->owner->id == current_process->id)  // double check !!
        {
            mut->locked_count++;    // it's okay we can acquire a mutex multiple time

            unlock_scheduler(&eFlags);
            return; 
        }

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

        //unlock_scheduler(&eFlags);

        block_task(WAITING); // I don't know if I should keep the scheduler locked or unlocked ...

        //unlock_scheduler(&eFlags);
    }

    mut->locked = true;
    mut->owner = current_process;   // new owner
    unlock_scheduler(&eFlags);
}

void release_mutex(mutex_t* mut)
{
    process_t* current_process = PROCESS_getCurrent();

    if(mut->owner != current_process || mut->owner->id != current_process->id)
    {
        fputs("mutex violation", VFS_FD_DEBUG);
        log_err("mutex", "Process id %d tried to release mutex it doesn't own! owner id: %d, mut 0x%x, current proc: 0x%x, owner proc: 0x%x, waiting first: 0x%x, waiting last: 0x%x", current_process->id, mut->owner->id, mut, current_process, mut->owner, mut->first_waiting_list, mut->last_waiting_list);
        panic();

        return;
    }

    uint32_t eFlags;
    lock_scheduler(&eFlags);

    if(mut->locked_count > 0)
    {
        mut->locked_count--;

        unlock_scheduler(&eFlags);
        return;
    }

    if(mut->first_waiting_list != NULL)
    {
        process_t* released = mut->first_waiting_list;

        mut->first_waiting_list = mut->first_waiting_list->next;

        mut->locked = true;
        mut->locked_count = 0;
        mut->owner = released;  // the ownner will be set again when the released task is scheduled

        unblock_task(released, false);

        unlock_scheduler(&eFlags);
        return;
    }
    else
    {
        mut->locked = false;
        mut->owner = NULL;
        mut->locked_count = 0;
        mut->first_waiting_list = NULL;
        mut->last_waiting_list = NULL;
    }
    unlock_scheduler(&eFlags);
}