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
#include <hal/heap.h>
#include <stdint.h>
#include <hal/virtmem_manager.h>
#include <memory.h>
#include <utility.h>
#include <hal/vmalloc.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define VMALLOC_START  0xD8000000
#define VMALLOC_END    0xDFFFFFFF

#define VMALLOC_SIZE (VMALLOC_END - VMALLOC_START)

#define BLOCK_SIZE 4096
#define BLOCK_PER_BYTE 8

typedef enum{
	FREE_BLOCK, // 0
    USED_BLOCK, // 1
}BITMAP_VALUE;

typedef struct tracking_list
{
    uint32_t addr;
    uint32_t block_size;
    struct tracking_list* next;
}tracking_list_t;

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

uint8_t* vmalloc_bitmap;
uint32_t vmalloc_totalBlockNumber   = 0;
uint32_t vmalloc_totalFreeBlock     = 0;
uint32_t vmalloc_totalUsedBlock     = 0;
uint32_t vmalloc_bitmapSize;

tracking_list_t* tracking_head = NULL;

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

void VMALLOC_setBlockToFree(uint32_t block)
{
    if(block > vmalloc_totalBlockNumber)
        return;
    
    vmalloc_bitmap[block / 8] ^= (1 << block % 8);
}

void VMALLOC_setBlockToUsed(int block)
{
    if(block > vmalloc_totalBlockNumber)
        return;
    
    vmalloc_bitmap[block / 8] |= (1 << block % 8);
}

uint8_t VMALLOC_checkIfBlockUsed(int block)
{
    if(block > vmalloc_totalBlockNumber)
        return 1;
    
    return vmalloc_bitmap[block / 8] & (1 << block % 8);
}

uint32_t VMALLOC_firstFreeBlock()
{
    for(int i = 0; i < vmalloc_totalBlockNumber; i++)
        if(VMALLOC_checkIfBlockUsed(i) == 0)
            return i;

    return -1;
}

uint32_t VMALLOC_firstFreeBlockFrom(uint32_t position)
{
    for(int i = position; i < vmalloc_totalBlockNumber; i++)
        if(VMALLOC_checkIfBlockUsed(i) == 0)
            return i;

    return -1;
}

void* VMALLOC_findFreeRange(uint32_t block_size)
{
    uint32_t index;
    uint32_t block_addr;
    uint32_t count;

    if(block_size > vmalloc_totalFreeBlock)
        return NULL;
    
    index = VMALLOC_firstFreeBlock();
    block_addr = index;
    count = 1; // we already have one block

    while (index < vmalloc_totalBlockNumber)
    {
        if(count >= block_size)
        {
            for(int i = 0; i < count; i++)
            {
                VMALLOC_setBlockToUsed(block_addr + i);
                vmalloc_totalUsedBlock++;
                vmalloc_totalFreeBlock--;
            }
            
            return (void*)((block_addr * BLOCK_SIZE) + VMALLOC_START);
        }

        index++;
        if(VMALLOC_checkIfBlockUsed(index))
        {
            count = 1;
            index = VMALLOC_firstFreeBlockFrom(index);
            block_addr = index;
        }

        count++;
    }
    
    return NULL;
}

void VMALLOC_freeThisRange(void* ptr, uint8_t size)
{
    if(!ptr)
        return;

    uint32_t block = (uint32_t)ptr / (BLOCK_SIZE);

    for(int i = 0; i < size; i++)
    {
        VMALLOC_setBlockToFree(block + i);
        vmalloc_totalUsedBlock--;
        vmalloc_totalFreeBlock++;
    }
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void VMALLOC_initialize()
{
    log_info("kernel", "Initializing vmalloc ...");

    vmalloc_totalBlockNumber = roundUp_div(VMALLOC_SIZE, BLOCK_SIZE);
    vmalloc_bitmapSize = roundUp_div(vmalloc_totalBlockNumber, BLOCK_PER_BYTE);

    vmalloc_bitmap = kmalloc(sizeof(uint8_t) * vmalloc_bitmapSize);
    if(vmalloc_bitmap == NULL)
    {
        log_err("kernel", "Initialization failed!\n");
        return; // error
    }

    // initialy we mark the whole memory as used
    memset(vmalloc_bitmap, 0b11111111, vmalloc_bitmapSize);

    // then we mark the availabe blocks
    for(int i = 0; i < vmalloc_totalBlockNumber; i++)
    {
        VMALLOC_setBlockToFree(i);
        vmalloc_totalFreeBlock++;
    }
}

void* vmalloc(size_t size)
{
    if(size <= 0)
        return NULL;

    uint32_t block_size = roundUp_div(size, BLOCK_SIZE);

    void* block_addr = VMALLOC_findFreeRange(block_size);

    for(int i = 0; i < block_size; i++)
        VIRTMEM_mapPage (block_addr + (BLOCK_SIZE * i), true);

    tracking_list_t* new = kmalloc(sizeof(tracking_list_t));

    new->addr = (uint32_t)block_addr;
    new->block_size = block_size;
    new->next = NULL;

    if(tracking_head == NULL)
        tracking_head = new;
    else
    {
        new->next = tracking_head->next;
        tracking_head->next = new;
    }

    return block_addr;
}

void vfree(void* ptr)
{
    if(ptr == NULL || tracking_head == NULL)
        return;

    tracking_list_t *this = tracking_head;
    tracking_list_t *before_this = NULL;
    while (this != NULL)
    {
        if(this->addr == (uint32_t)ptr)
        {
            VMALLOC_freeThisRange(ptr, this->block_size);
            for(int i = 0; i < this->block_size; i++)
                VIRTMEM_unMapPage(ptr + (BLOCK_SIZE * i));
            
            if(before_this != NULL)
            {
                before_this->next = this->next;
                kfree(this);
                this = NULL;
            }
            else
            {
                tracking_head = this->next;
                kfree(this);
            }
            
            return;
        }

        before_this = this;
        this = this->next;
    }

    // if it reaches here that's means we never allocated this pointer before !
}