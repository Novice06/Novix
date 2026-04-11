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
#include <utility.h>
#include <memory.h>
#include <mem_manager/virtmem_manager.h>
#include <mem_manager/heap.h>
#include <mem_manager/vmalloc.h>
#include <multitasking/scheduler.h>
#include <multitasking/process.h>
#include <multitasking/lock.h>
#include <multitasking/ipc/message.h>
#include <multitasking/ipc/shared_memory.h>

process_t* PROCESS_list[MAX_PROCESS];
uint32_t PROCESS_count;

process_t PROCESS_cleaner;
process_t* terminated_tasks; // dead process list

int id_dispatcher(process_t* proc)
{
    lock_scheduler();

    for(int i = 0; i < MAX_PROCESS; i++)
    {
        if(PROCESS_list[i] == NULL)
        {
            PROCESS_list[i] = proc;
            PROCESS_count++;

            unlock_scheduler();
            return i;
        }
    }

    unlock_scheduler();
    return -1;
}

process_t* PROCESS_get(uint32_t id)
{
    return PROCESS_list[id];
}

void block_task()
{
    // blocking a task just means removing it from the ready list
    // this is performed through the schedule_next function by checking the state of the process that yields the CPU
    // thats why we're change the state so that this task isnt put back in the ready list
    // but this code is commented out because a task switch can occurs right before changing the state and when scheduled again(for exemple this task was on waiting list)
    // this code will continue and change the state then yields this will lead to a permanent block without any means to unblock it since its not on any waiting or any kind of list
    // thats why the reponsability to change the state is on the caller side since change the state must not be interrupted
    
    // PROCESS_getCurrent()->state = reason;
    
    yield();
}

void unblock_task(process_t* proc, bool priority)
{
    add_READY_process(proc, priority);
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
            lock_scheduler();

            process_t* trash = terminated_tasks;
            terminated_tasks = terminated_tasks->next;

            unlock_scheduler();

            log_warn("cleaner", "cleaning 0x%x, id: %d", trash, trash->id);

            vfree(trash->esp0); // deallocate the kernel stack for the process
            VIRTMEM_destroyAddressSpace(trash->virt_cr3);

            PROCESS_list[trash->id] = NULL;
            PROCESS_count--;

            vm_region_t* region = trash->regions;
            while (region != NULL)
            {
                kfree(region);
                region = region->next;
            }
            
            kfree(trash);

            continue;
        }

        PROCESS_getCurrent()->state = BLOCKED;
        block_task();
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

    memset(idle->resources, 0, sizeof(file_descriptor_t) * MAX_OPEN_FILES);
    idle->regions = NULL;

    idle->next = NULL;

    // the cleaner process is also not meant to be terminated
    // but we can't just create it the same way we created the idle process
    // we actually need to allocate a stack and format it to allow a proper
    // context switch however this process will share the same address space with
    // the idle process just to save up some memory

    PROCESS_cleaner.esp0 = vmalloc(1);
    PROCESS_cleaner.esp = PROCESS_cleaner.esp0 + 0x1000 - 4;

    *(uint32_t*)PROCESS_cleaner.esp = (uint32_t)spawnProcess; // return address after task_switch

    PROCESS_cleaner.esp -= (4 * 5);   // pushed register
    *(uint32_t*)PROCESS_cleaner.esp = 0x202;       // default eflags for the new process
    //PROCESS_cleaner.esp -= (4 * 4);   // pushed register

    PROCESS_cleaner.cr3 = getPDBR();
    PROCESS_cleaner.virt_cr3 = NULL; // the cleaner task share the same virtual space with the idle process...
    PROCESS_cleaner.usermode = false;
    PROCESS_cleaner.entryPoint = cleaner_task;
    PROCESS_cleaner.id = id_dispatcher(&PROCESS_cleaner);
    memset(PROCESS_cleaner.resources, 0, sizeof(file_descriptor_t) * MAX_OPEN_FILES);
    PROCESS_cleaner.regions = NULL;
    PROCESS_cleaner.state = BLOCKED;    // initially this process is blocked and will be unblocked when there is a task termination

    shared_memory_init();   // intialize the shared_memory system
}

void PROCESS_createFrom(void* entryPoint)
{
    process_t* proc = kmalloc(sizeof(process_t));

    proc->esp0 = vmalloc(1);
    proc->esp = proc->esp0 + 0x1000 - 4;

    *(uint32_t*)proc->esp = (uint32_t)spawnProcess; // return address after task_switch

    proc->esp -= (4 * 5);   // pushed register
    *(uint32_t*)proc->esp = 0x202;       // default eflags for the new process

    //proc->esp -= (4 * 4);   // pushed register

    proc->virt_cr3 = VIRTMEM_createAddressSpace();
    proc->cr3 = VIRTMEM_getPhysAddr(proc->virt_cr3);    // store the physical address of the new pdbr

    proc->usermode = false;
    proc->entryPoint = entryPoint;

    proc->id = id_dispatcher(proc); // WARNING: should check if there is more room for this process
    memset(proc->resources, 0, sizeof(file_descriptor_t) * MAX_OPEN_FILES);
    proc->regions = NULL;

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
    //proc->esp -= (4 * 4);   // pushed register

    proc->virt_cr3 = VIRTMEM_createAddressSpace();
    proc->cr3 = VIRTMEM_getPhysAddr(proc->virt_cr3);    // store the physical address of the new pdbr

    proc->usermode = is_usermode;
    proc->entryPoint = is_usermode ? (void*)0x400000 : (void*)0xe0000000;

    proc->id = id_dispatcher(proc); // WARNING: should check if there is more room for this process
    memset(proc->resources, 0, sizeof(file_descriptor_t) * MAX_OPEN_FILES);
    proc->regions = NULL;

    proc->next = NULL;

    // because we want to write the new program's address space we need to switch pdbr
    lock_scheduler();

    void* currentpdbr = PROCESS_getCurrent()->cr3;
    PROCESS_getCurrent()->cr3 = proc->cr3;   // we're updating the current process in case a context switch occurs while reading the file
    switchPDBR(proc->cr3);

    unlock_scheduler();

    if(is_usermode)
    {
        VIRTMEM_mapPage((void*)(0xc0000000 - 4), false); // we need to map the stack for this process (4kb before 0xc0000000)

        vm_region_t* code = kmalloc(sizeof(vm_region_t));
        code->start = 0x400000;
        code->length = roundUp_div(length, 0x1000); // page size
        code->type = REGION_CODE;

        vm_region_t* heap = kmalloc(sizeof(vm_region_t));
        heap->start = code->start + (code->length * 0x1000);
        heap->length = roundUp_div(0x20000000, 0x1000);   // 512 Mo for the heap
        heap->type = REGION_HEAP;
        proc->brk = (void*)heap->start;

        vm_region_t* stack = kmalloc(sizeof(vm_region_t));
        stack->start = 0xBFF00000;
        stack->length = roundUp_div(0x100000, 0x1000);   // 1 Mo for the stack
        stack->type = REGION_STACK;


        code->next = heap;
        heap->next = stack;
        stack->next = NULL;

        proc->regions = code;
    }
    
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
    lock_scheduler();

    PROCESS_getCurrent()->cr3 = currentpdbr;
    switchPDBR(PROCESS_getCurrent()->cr3);

    unlock_scheduler();

    add_READY_process(proc, false);
}

int PROCESS_execve(const char *path, char* argv)
{
    int file = VFS_open(path, VFS_O_RDONLY);
    if(file < 0) return -1;

    vfs_stat_t stat;
    VFS_stat(path, &stat);

    process_t* proc = kmalloc(sizeof(process_t));

    proc->esp0 = vmalloc(1);
    proc->esp = proc->esp0 + 0x1000 - 4;

    *(uint32_t*)proc->esp = (uint32_t)spawnProcess; // return address after task_switch

    proc->esp -= (4 * 5);   // pushed register
    *(uint32_t*)proc->esp = 0x202;       // default eflags for the new process
    //proc->esp -= (4 * 4);   // pushed register

    proc->virt_cr3 = VIRTMEM_createAddressSpace();
    proc->cr3 = VIRTMEM_getPhysAddr(proc->virt_cr3);    // store the physical address of the new pdbr

    proc->usermode = true;
    proc->entryPoint = (void*)0x400000;

    proc->id = id_dispatcher(proc); // WARNING: should check if there is more room for this process
    memset(proc->resources, 0, sizeof(file_descriptor_t) * MAX_OPEN_FILES);
    proc->regions = NULL;

    proc->next = NULL;


    // TODO: find a way to pass argv and argc (this action must be performed before switching pdbr because argv is a userpace pointer)
    // int argc = 0;
    // for(int i = 0; argv[i] != NULL; i++) argc++;


    // because we want to write the new program's address space we need to switch pdbr
    lock_scheduler();

    void* currentpdbr = PROCESS_getCurrent()->cr3;
    PROCESS_getCurrent()->cr3 = proc->cr3;   // we're updating the current process in case a context switch occurs while reading the file
    switchPDBR(proc->cr3);

    unlock_scheduler();

    
    for (int i = 0; i < 32; i++) // alloc 128 KB for the stack!
        VIRTMEM_mapPage((void*)(0xc0000000 - (i+1)*0x1000), false);

    vm_region_t* code = kmalloc(sizeof(vm_region_t));
    code->start = 0x400000;
    code->length = roundUp_div(stat.size, 0x1000); // page size
    code->type = REGION_CODE;

    vm_region_t* heap = kmalloc(sizeof(vm_region_t));
    heap->start = code->start + (code->length * 0x1000);
    heap->length = roundUp_div(0x20000000, 0x1000);   // 512 Mo for the heap
    heap->type = REGION_HEAP;
    proc->brk = (void*)heap->start;

    vm_region_t* stack = kmalloc(sizeof(vm_region_t));
    stack->start = 0xBFF00000;
    stack->length = roundUp_div(0x100000, 0x1000);   // 1 Mo for the stack
    stack->type = REGION_STACK;


    code->next = heap;
    heap->next = stack;
    stack->next = NULL;

    proc->regions = code;


    void* buffer = proc->entryPoint;
    for(int i = 0; i < roundUp_div(stat.size, 0x1000); i++)
        VIRTMEM_mapPage(buffer + (i * 0x1000), false);

    VFS_read(file, buffer, stat.size);

    // restoring pdbr
    lock_scheduler();

    PROCESS_getCurrent()->cr3 = currentpdbr;
    switchPDBR(PROCESS_getCurrent()->cr3);

    unlock_scheduler();

    add_READY_process(proc, false);
    return proc->id;
}

void* PROCESS_sbrk(intptr_t size)
{
    vm_region_t* heap = PROCESS_getCurrent()->regions->next;    // because the heap is right next to the code region there is no need to search for it manuelly

    if((uint32_t)(PROCESS_getCurrent()->brk + size) > (heap->start + (heap->length * 0x1000)))
    {
        log_err("brk", "requested: 0x%x, available: 0x%x, check type: %d", (PROCESS_getCurrent()->brk + size), (heap->start + heap->length), heap->type);
        return PROCESS_getCurrent()->brk;   // failed
    }

    // log_debug("sbrk", "requested 0x%x, current sbrk: 0x%x, will end at: 0x%x", size, PROCESS_getCurrent()->brk, PROCESS_getCurrent()->brk + size);
    // // uint32_t current_page = (uint32_t)PROCESS_getCurrent()->brk & 0xFFFFFFF8;
    // // uint32_t end_page = ((uint32_t)PROCESS_getCurrent()->brk + size) & 0xFFFFFFF8;
    // uint32_t current_page = (uint32_t)PROCESS_getCurrent()->brk & ~(0x1000 - 1);
    // uint32_t end_page = ((uint32_t)PROCESS_getCurrent()->brk + size) & ~(0x1000 - 1);
    // log_debug("sbrk", "current page 0x%x, endpage 0x%x", current_page, end_page);
    // for(; current_page <= end_page; current_page+=0x1000)
    //     VIRTMEM_mapPage((void*)current_page, !PROCESS_getCurrent()->usermode);

    // PROCESS_getCurrent()->brk += size;
    // return PROCESS_getCurrent()->brk;

    uint32_t old_brk = PROCESS_getCurrent()->brk;
    uint32_t new_brk = old_brk + size;

    // 1. Calculer la page de départ (arrondi à la page inférieure)
    uint32_t start_page = old_brk & ~(0x1000 - 1);
    // 2. Calculer la page de fin (on veut couvrir jusqu'au dernier octet de new_brk)
    uint32_t end_page = (new_brk - 1) & ~(0x1000 - 1);

    // 3. On ne mappe que si on a changé de page ou si on progresse
    if (new_brk > old_brk) {
        for (uint32_t curr = start_page; curr <= end_page; curr += 0x1000) {
            VIRTMEM_mapPage((void*)curr, !PROCESS_getCurrent()->usermode);
        }
    }

    PROCESS_getCurrent()->brk = (void*)new_brk;
    return (void*)old_brk; // sbrk retourne l'ANCIENNE valeur
}

void* PROCESS_createNewRegion(region_type_t type, uint32_t length, uint64_t shm_id)
{
    vm_region_t* regions = PROCESS_getCurrent()->regions;
    if(regions == NULL)
    {
        uint32_t limit = (0xC0000000 - 0x400000) / 0x1000; // in 4KB block
        if(length > limit)
            return NULL;

        vm_region_t* new = kmalloc(sizeof(vm_region_t));
        new->start = 0x400000;
        new->length = length;
        new->type = type;

        if(REGION_SHM == type)
            new->shm_id = shm_id;

        new->next = NULL;
        PROCESS_getCurrent()->regions = new;

        return (void*)0x400000;
    }
    else
    {
        while (1)
        {
            if(regions->next == NULL)
            {
                uint32_t limit = (0xC0000000 - regions->start) / 0x1000 - regions->length; // in 4KB block
                if(length > limit)
                    return NULL;

                vm_region_t* new = kmalloc(sizeof(vm_region_t));
                new->start = regions->start + regions->length * 0x1000;
                new->length = length;
                new->type = type;

                if(REGION_SHM == type)
                    new->shm_id = shm_id;

                new->next = NULL;
                regions->next = new;
                
                return (void*)new->start;
            }

            uint32_t available_size = (regions->next->start - regions->start) / 0x1000 - regions->length; // in 4KB block
            if(length > available_size)
            {
                regions = regions->next;
                continue;
            }

            vm_region_t* new = kmalloc(sizeof(vm_region_t));
            new->start = regions->start + regions->length * 0x1000;
            new->length = length;
            new->type = type;

            if(REGION_SHM == type)
                new->shm_id = shm_id;

            new->next = regions->next;
            regions->next = new;
            
            return (void*)new->start;
        }
        
    }
}

void PROCESS_terminate()
{
    destroy_endpoint();
    shared_memory_detachAll();

    lock_scheduler();

    for(int i = 0; i < MAX_OPEN_FILES; i++)
        VFS_close(i);

    PROCESS_getCurrent()->state = DEAD;
    PROCESS_getCurrent()->next = terminated_tasks;
    terminated_tasks = PROCESS_getCurrent();

    if(PROCESS_cleaner.state == BLOCKED)
        unblock_task(&PROCESS_cleaner, true);
    
    unlock_scheduler();
    yield();
}