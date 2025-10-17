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
#include <multitasking/process.h>
#include <multitasking/scheduler.h>
#include <mem_manager/heap.h>
#include <mem_manager/vmalloc.h>
#include <mem_manager/virtmem_manager.h>
#include <memory.h>
#include <utility.h>

#define MAX_PROCESS (1024 * 1024)

void __attribute__((cdecl)) switch_to_usermode(uint32_t stack, uint32_t ip);

process_t* PROCESS_list[MAX_PROCESS];
uint32_t PROCESS_count;

process_t PROCESS_cleaner;
process_t* terminated_tasks; // dead process list

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

void block_task(status_t reason)
{
    PROCESS_getCurrent()->state = reason;
    
    yield();
}

void unblock_task(process_t* proc)
{
    add_READY_process(proc, true);
}

void spawnProcess()
{
    void (*entryPoint)();

    entryPoint = PROCESS_getCurrent()->entryPoint;
    
    if(PROCESS_getCurrent()->usermode)
        switch_to_usermode(0xc0000000, (uint32_t)0x400000);
    else
        entryPoint();
}

void cleaner_task()
{
    while (true)
    {
        if(terminated_tasks != NULL)
        {
            process_t* trash = terminated_tasks;
            terminated_tasks = terminated_tasks->next;

            log_debug("cleaner", "cleaning 0x%x, id: %d", trash, trash->id);

            vfree(trash->esp0); // deallocate the kernel stack for the process
            VIRTMEM_destroyAddressSpace(trash->virt_cr3);

            PROCESS_list[trash->id] = NULL;
            PROCESS_count--;

            kfree(trash);

            continue;
        }

        block_task(BLOCKED);
        yield();
    }
    
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

    // the cleaner process is also not meant to be terminated
    // but we can't just create it the same way we created the idle process
    // we actually need to allocate a stack and format it to allow a proper
    // context switch however this process will share the same address space with
    // the idle process just to save up memory

    PROCESS_cleaner.esp0 = vmalloc(1);
    PROCESS_cleaner.esp = PROCESS_cleaner.esp0 + 0x1000 - 4;

    *(uint32_t*)PROCESS_cleaner.esp = (uint32_t)spawnProcess; // return address after task_switch

    PROCESS_cleaner.esp -= (4 * 5);   // pushed register
    *(uint32_t*)PROCESS_cleaner.esp = 0x202;       // default eflags for the new process

    PROCESS_cleaner.cr3 = getPDBR();
    PROCESS_cleaner.virt_cr3 = NULL; // the cleaner task share the same virtual space with the idle process...
    PROCESS_cleaner.usermode = false;
    PROCESS_cleaner.entryPoint = cleaner_task;
    PROCESS_cleaner.id = id_dispatcher(&PROCESS_cleaner);
    PROCESS_cleaner.state = BLOCKED;    // initially this process is blocked and will be unblocked when there is a task termination
}

void PROCESS_createFrom(void* entryPoint)
{
    process_t* proc = kmalloc(sizeof(process_t));

    proc->esp0 = vmalloc(1);
    proc->esp = proc->esp0 + 0x1000 - 4;

    *(uint32_t*)proc->esp = (uint32_t)spawnProcess; // return address after task_switch

    proc->esp -= (4 * 5);   // pushed register
    *(uint32_t*)proc->esp = 0x202;       // default eflags for the new process

    proc->virt_cr3 = VIRTMEM_createAddressSpace();
    proc->cr3 = VIRTMEM_getPhysAddr(proc->virt_cr3);    // store the physical address of the new pdbr

    proc->usermode = false;
    proc->entryPoint = entryPoint;

    proc->id = id_dispatcher(proc); // WARNING: should check if there is more room for this process

    proc->next = NULL;

    add_READY_process(proc, false);
}

void PROCESS_createFromByteArray(void* array, int length, bool is_usermode)
{
    process_t* proc = kmalloc(sizeof(process_t));

    proc->esp0 = vmalloc(1);
    proc->esp = proc->esp0 + 0x1000 - 4;

    *(uint32_t*)proc->esp = (uint32_t)spawnProcess; // return address after task_switch

    proc->esp -= (4 * 5);   // pushed register
    *(uint32_t*)proc->esp = 0x202;       // default eflags for the new process

    proc->virt_cr3 = VIRTMEM_createAddressSpace();
    proc->cr3 = VIRTMEM_getPhysAddr(proc->virt_cr3);    // store the physical address of the new pdbr

    proc->usermode = is_usermode;
    proc->entryPoint = is_usermode ? (void*)0x400000 : (void*)0xe0000000;

    proc->id = id_dispatcher(proc); // WARNING: should check if there is more room for this process

    proc->next = NULL;

    // because we want to write the new program's address space we need to switch pdbr
    void* currentpdbr = PROCESS_getCurrent()->cr3;
    PROCESS_getCurrent()->cr3 = proc->cr3;   // we're updating the current process in case a context switch occurs while reading the file
    switchPDBR(proc->cr3);

    if(is_usermode)
        VIRTMEM_mapPage((void*)(0xc0000000 - 4), false); // we need to map the stack for this process (4kb before 0xc0000000)
    
    /*
     * WARNING !! because 0xe0000000 is a higher half address it's identity mapped in every process
     * We need to map this address to a different physical address, because the one currently there
     * might point to another kernel process that was already present. This issue can be resolved 
     * by fixing the VIRTMEM_createAddressSpace() function so that it memsets the kernel process address range to zero,
     * and by also making sure that this range is properly unmapped in the VIRTMEM_destroyAddressSpace() function.
     * However, since I’m feeling a bit lazy right now and don’t want to mess with the virtual memory manager code, 
     * I’ll leave things as they are, especially since I don’t have any external drivers or kernel program to load at the moment.
    */

    void* buffer = is_usermode ? (void*)0x400000 : (void*)0xe0000000;

    for(int i = 0; i < roundUp_div(length, 0x1000); i++)
        VIRTMEM_mapPage(buffer + (i * 0x1000), false);

    memcpy(buffer, array, length);

    // restoring pdbr
    PROCESS_getCurrent()->cr3 = currentpdbr;
    switchPDBR(PROCESS_getCurrent()->cr3);

    add_READY_process(proc, false);
}

void PROCESS_terminate()
{
    PROCESS_getCurrent()->state = DEAD;

    PROCESS_getCurrent()->next = terminated_tasks;
    terminated_tasks = PROCESS_getCurrent();

    if(PROCESS_cleaner.state == BLOCKED)
        unblock_task(&PROCESS_cleaner);
    
    yield();
}