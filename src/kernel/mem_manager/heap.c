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

#include <memory.h>
#include <mem_manager/virtmem_manager.h>
#include <mem_manager/heap.h>
#include <ordered_array.h>
#include <multitasking/scheduler.h>
#include <multitasking/lock.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define HEAP_START_ADDR 0xD0000000
#define HEAP_END_ADDR   0xD7FFFFFF

#define PAGE_SIZE 0x1000

#define FREEBLOCK_LIST_SIZE PAGE_SIZE
#define BREAK_START_ADDR    (HEAP_START_ADDR + FREEBLOCK_LIST_SIZE)

typedef struct header_t header_t;
struct header_t{
    size_t size;
    bool isFree;
    header_t *next;
    header_t *back;
};

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

void* brk = NULL;
uint32_t lastHeapAllocatedPage;

header_t *head = NULL, *tail = NULL;
ordered_array freeBlockArray;

mutex_t HEAP_mutex;

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

header_t* search_freeBlock(size_t size)
{
    header_t *header;

    for(int i = 0; i < freeBlockArray.size; i++) // search a free block from the free block array
    {
        header = (header_t*)freeBlockArray.array[i];
        if(header->size >= size && header->isFree)
            return header;
    }

    return NULL;
}

bool criteria_function(type_t a, type_t b)
{
    return ((header_t*)a)->size < ((header_t*)b)->size;   // ordered by size in ascending order
}

void* sbrk(intptr_t size)
{
    void* ptr = brk;

    if(size == 0)
    {
        return brk;
    }
    else if(size > 0)
    {
        if((uint32_t)(brk+size) > HEAP_END_ADDR)
        {
            return (void*)-1;   // not enough available memory, heap is full !
        }

        if((uint32_t)(brk+size) > (lastHeapAllocatedPage + PAGE_SIZE)) // if so we will need to increase the heap size
        {
            // we need to map this new page first
            if(!VIRTMEM_mapPage((void*)(lastHeapAllocatedPage + PAGE_SIZE), true))
                return (void*)-1;   // not enough available memory, RAM is full or other error !

            lastHeapAllocatedPage += PAGE_SIZE;
        }
    }
    else
    {
        if((uint32_t)(brk+size) < BREAK_START_ADDR)
            return (void*)-1;

        // reminder: the size is negative
        if((uint32_t)(brk+size) < lastHeapAllocatedPage)
        {
            VIRTMEM_unMapPage((void*)lastHeapAllocatedPage);    // releasing memory
            lastHeapAllocatedPage -= PAGE_SIZE;
        }
    }

    brk += size;
    return ptr;
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

bool HEAP_initialize()
{
    // the first 4kb of the heap range is reserved for the free block array
    // therefore the value of BREAK_START_ADDR
    brk = (void*)BREAK_START_ADDR;
    lastHeapAllocatedPage = HEAP_START_ADDR;

    // first we need to allocate all the page table for the heap address range
    // because we want it to be consistent (to be the same) in all address space
    for(uint32_t i = HEAP_START_ADDR; i <= HEAP_END_ADDR; i += (400 * 0x1000))
        VIRTMEM_mapTable((void*)i, true);

    if(!VIRTMEM_mapPage((void*)lastHeapAllocatedPage, true)) // first we map the freeBlockArray address
        return false;

    lastHeapAllocatedPage += PAGE_SIZE;

    if(!VIRTMEM_mapPage((void*)lastHeapAllocatedPage, true)) // then we map the actual working heap address
        return false;

    freeBlockArray = create_static_array((void*)HEAP_START_ADDR, FREEBLOCK_LIST_SIZE / sizeof(type_t), criteria_function);

    return true;
}

void* kmalloc(size_t size)
{
    void *block;
    header_t *header = NULL;
    size_t totalSize = sizeof(header_t) + size;

    if(!size)
        return NULL;

    if(is_schedulerEnabled())
        acquire_mutex(&HEAP_mutex);

    header = search_freeBlock(size);
    if(header)
    {
        header->isFree = false;
        remove_ordered_array(getIndex_ordered_array(header, &freeBlockArray), &freeBlockArray); // remove from free block list

        if((header->size - size) >= (sizeof(header_t) + 16)) // we split the block
        {
            header_t* newHeader = (header_t*)((void*)header + size + sizeof(header_t));

            //filling new header information
            newHeader->size = header->size - size - sizeof(header_t);
            newHeader->isFree = false;
            newHeader->back = header;
            newHeader->next = header->next;

            if(header->next != NULL)
                header->next->back = newHeader;

            header->next = newHeader;
            header->size = size;

            kfree((void*)newHeader + sizeof(header_t)); // add a new free block !

            if(header == tail)
                tail = newHeader;
        }

        if(is_schedulerEnabled())
            release_mutex(&HEAP_mutex);

        return ((void*)header + sizeof(header_t));
    }

    block = sbrk(totalSize);    // requesting memory from the heap
    if(block == (void*) -1)
    {
        if(is_schedulerEnabled())
            release_mutex(&HEAP_mutex);

        return NULL;
    }

    //filling header information
    header = (header_t*)block;
    header->size = size;
    header->isFree = false;
    header->next = NULL;
    header->back = NULL;

    // now our linked list of allocated block
    if(!head)
        head = header;
    
    if(tail)
    {
        tail->next = header;
        header->back = tail;
    }

    tail = header;

    block += sizeof(header_t);

    if(is_schedulerEnabled())
        release_mutex(&HEAP_mutex);

    return block;
}

void* krealloc(void* block, size_t size)
{
    if(block == NULL)
        return kmalloc(size);
    
    header_t* header = block - sizeof(header_t);
    void* newBlock;

    if(header->size >= size)
        return block;

    newBlock = kmalloc(size);
    if(!newBlock)
        return NULL;

    memcpy(newBlock, block, header->size);
    kfree(block);

    return newBlock;
}

void* kcalloc(size_t num, size_t size)
{
    size_t totalSize = num * size;

    if(!num || !size)
        return NULL;

    if(num != totalSize / size) // check mul overflow 
        return NULL;

    void* pointer = kmalloc(totalSize);
    if(!pointer)
        return NULL;

    memset(pointer, 0, totalSize);
    
    return pointer;
}

void kfree(void* block)
{
    if(block == NULL)
        return;
    
    header_t *left_block, *right_block;
    header_t *header = (header_t*)(block - sizeof(header_t));
    size_t totalSize = 0;

    if(header->isFree)
        return; // nothing to do

    if(is_schedulerEnabled())
        acquire_mutex(&HEAP_mutex);

    // merging left block if it's free
    left_block = header->back;
    if(left_block != NULL && left_block->isFree)
    {
        remove_ordered_array(getIndex_ordered_array(left_block, &freeBlockArray), &freeBlockArray);

        left_block->size += header->size + sizeof(header_t);
        left_block->next = header->next;
        
        if(header->next != NULL)
            header->next->back = left_block;

        if(header == tail)      // if we are merging the last block
            tail = left_block; // the last block become the left one

        header = left_block;    // the actual header now to keep the right merge and the rest of the function consistent
    }

    //merging right block if it's free
    right_block = header->next;
    if(right_block != NULL && right_block->isFree)
    {
        remove_ordered_array(getIndex_ordered_array(right_block, &freeBlockArray), &freeBlockArray);

        header->size += right_block->size + sizeof(header_t);
        header->next = right_block->next;
        
        if(right_block->next != NULL)
            right_block->next->back = header;

        if(right_block == tail) // if we are merging the last block
            tail = header;      // the last block become the left one in this case the header
    }

    header->isFree = true;
    insert_ordered_array((type_t)header, &freeBlockArray);

    totalSize = sizeof(header_t) + header->size;

    // if it's the last block we need to release the memory to the OS
    if(header == tail)      //(block + header->size) == sbrk(0)
    {
        if(tail == head)    // if it's the first element in the linked list
        {
            tail = NULL;    // erase the actual linked list
            head = NULL;
            sbrk(-1 * totalSize);
        }
        else
        {
            tail = header->back;
            tail->next = NULL;

            sbrk(-1 * totalSize);
        }

        remove_ordered_array(getIndex_ordered_array(header, &freeBlockArray), &freeBlockArray); // in all case we need to remove it from the free list array
    }

    if(is_schedulerEnabled())
        release_mutex(&HEAP_mutex);
}