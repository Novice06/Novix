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
#include <mem_manager/virtmem_manager.h>
#include <mem_manager/heap.h>
#include <mem_manager/vmalloc.h>
#include <multitasking/scheduler.h>
#include <multitasking/process.h>

#define MAX_PROCESS (1024 * 1024)

process_t* PROCESS_list[MAX_PROCESS];
uint32_t PROCESS_count;

int id_dispatcher(process_t* proc)
{
    for(int i = 0; i < MAX_PROCESS; i++)
    {
        if(PROCESS_list[i] == NULL)
        {
            PROCESS_list[i] = proc;
            PROCESS_count++;
            return i;
        }
    }

    return -1;
}

void spawnProcess()
{
    void (*entryPoint)();

    entryPoint = PROCESS_getCurrent()->entryPoint;
    
    // if(PROCESS_getCurrent()->usermode)
    //     switch_to_usermode(0xc0000000 - 4, (uint32_t)0x400000);
    // else
    //     entryPoint();

    entryPoint();
}

void PROCESS_initialize(process_t* idle)
{
    // create the first process which is the idle process
    // the idle process represent the current stream of execution
    // and it's idle because it will be the one executing when no
    // other process is available to use the CPU

    idle->esp0 = NULL;  // doesn't have esp0 because this process is never meant to be terminated (which involves freeing the stack)
    idle->esp = NULL;   // this will be filled automatically when a context switch occurs

    idle->cr3 = getPDBR();
    idle->virt_cr3 = NULL;  // same reason as esp0
    idle->usermode = false;
    idle->entryPoint = NULL;
    idle->id = id_dispatcher(idle);

    idle->next = NULL;
}

void PROCESS_createAndSchedule(void* entryPoint, bool is_usermode)
{
    process_t* proc = kmalloc(sizeof(process_t));

    proc->esp0 = vmalloc(1);
    proc->esp = proc->esp0 + 0x1000 - 4;

    *(uint32_t*)proc->esp = (uint32_t)spawnProcess; // return address after task_switch

    proc->esp -= (4 * 5);   // pushed register
    *(uint32_t*)proc->esp = 0x202;       // default eflags for the new process

    // proc->virt_cr3 = VIRTMEM_createAddressSpace();
    // proc->cr3 = VIRTMEM_getPhysAddr(proc->virt_cr3);    // store the physical address of the new pdbr

    proc->virt_cr3 = NULL;
    proc->cr3 = getPDBR();    // store the physical address of the new pdbr

    proc->usermode = is_usermode;
    proc->entryPoint = entryPoint;

    proc->id = id_dispatcher(proc); // WARNING: should check if there is more room for this process

    proc->next = NULL;

    add_READY_process(proc, false);
}