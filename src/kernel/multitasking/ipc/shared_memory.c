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

#include <stdint.h>
#include <debug.h>
#include <list.h>
#include <utility.h>
#include <feistel64.h>
#include <mem_manager/physmem_manager.h>
#include <mem_manager/virtmem_manager.h>
#include <mem_manager/heap.h>
#include <multitasking/time.h>
#include <multitasking/lock.h>
#include <multitasking/scheduler.h>

typedef struct shared_memory
{
    uint64_t id;
    void* phys_base;
    uint32_t length;    // in 4KB blocks
    int ref_count;
}shared_memory_t;

list_t* memories_shared;
mutex_t* mutex_list;

static uint64_t shm_counter = 1;
static uint64_t shm_key;

void shared_memory_init()
{
    shm_key = get_tikCount();
    memories_shared = create_newList();
    mutex_list = create_mutex();
}

uint64_t shared_memory_create(uint32_t length)
{
    // reject if request a 0 length

    shared_memory_t* new = kmalloc(sizeof(shared_memory_t));

    new->length = roundUp_div(length, 0x1000);
    new->ref_count = 0;
    new->phys_base = PHYSMEM_AllocBlocks(new->length);
    new->id = feistel64(shm_counter++, shm_key);

    acquire_mutex(mutex_list);
    list_add(memories_shared, new);
    release_mutex(mutex_list);

    return new->id;
}

void* shared_memory_attach(uint64_t id)
{
    shared_memory_t* mem = NULL;

    acquire_mutex(mutex_list);
    for(uint32_t i = 0; i < memories_shared->count; i++)
    {

        if(((shared_memory_t*)list_getAt(memories_shared, i))->id == id)
        {
            mem = list_getAt(memories_shared, i);
            break;
        }
            
    }
    release_mutex(mutex_list);

    if(!mem)
    {
        log_debug("share_memory_attach", "Holy Molly whats going on ? cound: %d", memories_shared->count);
        return NULL;
    }

    void* mem_base = PROCESS_createNewRegion(REGION_SHM, mem->length, mem->id);

    if(NULL == mem_base)    // there is no free region
        return NULL;
        
    for(uint32_t i = 0; i < mem->length; i++)
        VIRTMEM_mapPageCustom(mem->phys_base + (i * 0x1000), mem_base + (i * 0x1000), !PROCESS_getCurrent()->usermode);

    mem->ref_count++;
    return mem_base;
}

void shared_memory_detach(uint64_t id)
{
    shared_memory_t* mem = NULL;

    acquire_mutex(mutex_list);
    for(uint32_t index = 0; index < memories_shared->count; index++)
    {

        if(((shared_memory_t*)list_getAt(memories_shared, index))->id == id)
        {
            mem = list_getAt(memories_shared, index);
            break;
        }
            
    }
    release_mutex(mutex_list);

    if(!mem)
        return; // that shared memory doesn't exist

    vm_region_t* region = PROCESS_getCurrent()->regions;
    vm_region_t* before = NULL;
    while(region != NULL)
    {
        if(region->type == REGION_SHM && region->shm_id == id)
            break;

        before = region;
        region = region->next;
    }
    
    if(NULL != region)
    {
        for(uint32_t i = 0; i < mem->length; i++)
            VIRTMEM_unMapPageCustom((void*)(region->start + (i * 0x1000)));

        if(NULL == before)  // its the first in the list
        {
            PROCESS_getCurrent()->regions = region->next;
        }
        else
        {
            before->next = region->next;
        }

        kfree(region);
        mem->ref_count--;

        if(0 == mem->ref_count)
        {
            acquire_mutex(mutex_list);
            for(uint32_t index = 0; index < memories_shared->count; index++)
            {

                if(((shared_memory_t*)list_getAt(memories_shared, index))->id == id)
                {
                    list_removeAt(memories_shared, index);    // this time we remove it from the list
                    break;
                }
                    
            }
            release_mutex(mutex_list);

            PHYSMEM_freeBlocks(mem->phys_base, mem->length);
            kfree(mem);
        }
    }
}

void shared_memory_detachAll()
{
    vm_region_t* region = PROCESS_getCurrent()->regions;
    while(region != NULL)
    {
        if(region->type == REGION_SHM)
        {
            uint64_t id = region->shm_id;
            region = region->next;  // because this region is about to be removed

            shared_memory_detach(id);
            continue;
        }

        region = region->next;
    }
}