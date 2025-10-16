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
#include <stdbool.h>
#include <stddef.h>
#include <mem_manager/physmem_manager.h>
#include <memory.h>
#include <utility.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define KERNEL_BASE_ADDR 0x100000
#define MAX_MEMORY_ENTRY 256
#define BLOCK_SIZEKB 4
#define BLOCK_PER_BYTE 8

typedef enum{
	FREE_BLOCK, // 0
    USED_BLOCK, // 1
}BITMAP_VALUE;

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

// memory map but in 4kb size
Memory_mapEntry g_memory4KbEntries[MAX_MEMORY_ENTRY];

// useful data for our memory manager
uint8_t* PHYSMEM_bitmap;
uint32_t PHYSMEM_totalBlockNumber   = 0;
uint32_t PHYSMEM_totalFreeBlock     = 0;
uint32_t PHYSMEM_totalUsedBlock     = 0;
uint32_t PHYSMEM_bitmapSize;

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================

bool PHYSMEM_initData(Boot_info* info, uint32_t kernel_size);
void PHYSMEM_memoryMapToBlock(uint32_t memoryBlockCount);
void PHYSMEM_setBlockToFree(uint32_t block);
void PHYSMEM_setBlockToUsed(int block);
bool PHYSMEM_checkIfBlockUsed(int block);
uint32_t PHYSMEM_firstFreeBlock();
uint32_t PHYSMEM_firstFreeBlockFrom(uint32_t position);

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

/*
 * this function initialize some of the useful data of the memory manager
 * sucha as where are we storing the bitmap, it's size ect.
*/ 
bool PHYSMEM_initData(Boot_info* info, uint32_t kernel_size)
{
    uint32_t base;

    PHYSMEM_totalBlockNumber = roundUp_div(info->memorySize, BLOCK_SIZEKB);
    PHYSMEM_bitmapSize = roundUp_div(PHYSMEM_totalBlockNumber, BLOCK_PER_BYTE);

    // here we are trying to find a free block of memory for the bitmap
    for(int i = 0; i < info->memoryBlockCount; i++)
    {
        if(info->memoryBlockEntries[i].type == AVAILABLE && info->memoryBlockEntries[i].length >= PHYSMEM_bitmapSize)
        {
            base = info->memoryBlockEntries[i].base;
            goto Found;
        }
    }

    return false;   // we can't find enough space for the bitmap? is this a joke?

Found:
    PHYSMEM_bitmap = (uint8_t*)base;

    // initialy we mark the whole memory as used
    memset(PHYSMEM_bitmap, 0b11111111, PHYSMEM_bitmapSize);

    // to prevent allocating and overwriting this region
    // we need to add a new reserved region to our memory map (because that's where the bitmap will reside)
    info->memoryBlockEntries[info->memoryBlockCount].base = base;
    info->memoryBlockEntries[info->memoryBlockCount].length = PHYSMEM_bitmapSize;
    info->memoryBlockEntries[info->memoryBlockCount].type = RESERVED;

    info->memoryBlockCount++;

    // now, very important we need to add a new reserved region for where the kernel has been loaded into
    info->memoryBlockEntries[info->memoryBlockCount].base = KERNEL_BASE_ADDR;
    info->memoryBlockEntries[info->memoryBlockCount].length = kernel_size;
    info->memoryBlockEntries[info->memoryBlockCount].type = RESERVED;

    info->memoryBlockCount++;

    // the memory map to the 4kb size array
    memcpy(&g_memory4KbEntries, info->memoryBlockEntries, MAX_MEMORY_ENTRY);

    return true;
}

// this convert the memory map entry to 4kb size
void PHYSMEM_memoryMapToBlock(uint32_t memoryBlockCount)
{
    /* 
    * the way we proceed here is:
    * - For all AVAILABLE blocks, we divide the base and length into 4KB chunks,
    *   and we round UP the base address to ensure 4KB alignment.
    *
    * - For any other block types (e.g., reserved), we round UP the **length** only,
    *   keeping the base untouched.
    *
    * This is necessary because we want reserved blocks to preserve their entire range,
    * while free blocks should be usable starting from a clean 4KB-aligned base.
    */

	for(int i = 0; i < memoryBlockCount; i++)
	{
		if(g_memory4KbEntries[i].type == AVAILABLE)
		{
			g_memory4KbEntries[i].base = roundUp_div(g_memory4KbEntries[i].base, BLOCK_SIZEKB * 0x400);
			g_memory4KbEntries[i].length = g_memory4KbEntries[i].length / (BLOCK_SIZEKB * 0x400);
		}else{
			g_memory4KbEntries[i].base = g_memory4KbEntries[i].base / (BLOCK_SIZEKB * 0x400);
			g_memory4KbEntries[i].length = roundUp_div(g_memory4KbEntries[i].length, BLOCK_SIZEKB * 0x400);
		}
	}
}

void PHYSMEM_setBlockToFree(uint32_t block)
{
    if(block > PHYSMEM_totalBlockNumber)
        return;
    
    PHYSMEM_bitmap[block / 8] ^= (1 << block % 8);
}

void PHYSMEM_setBlockToUsed(int block)
{
    if(block > PHYSMEM_totalBlockNumber)
        return;
    
    PHYSMEM_bitmap[block / 8] |= (1 << block % 8);
}

bool PHYSMEM_checkIfBlockUsed(int block)
{
    if(block > PHYSMEM_totalBlockNumber)
        return true;
    
    return (PHYSMEM_bitmap[block / 8] & (1 << block % 8)) ? true : false;
}

uint32_t PHYSMEM_firstFreeBlock()
{
    for(int i = 0; i < PHYSMEM_totalBlockNumber; i++)
        if(!PHYSMEM_checkIfBlockUsed(i))
            return i;

    return -1;
}

uint32_t PHYSMEM_firstFreeBlockFrom(uint32_t position)
{
    for(int i = position; i < PHYSMEM_totalBlockNumber; i++)
        if(!PHYSMEM_checkIfBlockUsed(i))
            return i;

    return -1;
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

bool PHYSMEM_initialize(Boot_info* info, uint32_t kernel_size)
{
    uint32_t block;

    if(info == NULL)
        return false;

    if(!PHYSMEM_initData(info, kernel_size))
        return false;


    // transform the actual memory map to a memory map with bases and length align to 4Kb
    PHYSMEM_memoryMapToBlock(info->memoryBlockCount);

    // because init data has already marked the hole memory map as used 
    // all that we need now is to mark where are free blocks first
    for(int i = 0; i < info->memoryBlockCount; i++)
    {
        if(g_memory4KbEntries[i].type == AVAILABLE)
        {
            for(int j = 0; j < g_memory4KbEntries[i].length; j++)
            {
                block = g_memory4KbEntries[i].base + j;
                PHYSMEM_setBlockToFree(block);
            }
        }
    }

    // and then only after the previous step we need to mark reserved blocks
    // this is required to prevent things like overlapping blocks
    for(int i = 0; i < info->memoryBlockCount; i++)
    {
        if(g_memory4KbEntries[i].type != AVAILABLE)
        {
            for(int j = 0; j < g_memory4KbEntries[i].length; j++)
            {
                block = g_memory4KbEntries[i].base + j;
                PHYSMEM_setBlockToUsed(block);
            }
        }
    }

    for(int i = 0; i < PHYSMEM_totalBlockNumber; i++)
    {
        if(!PHYSMEM_checkIfBlockUsed(i))
            PHYSMEM_totalFreeBlock++;
        else
            PHYSMEM_totalUsedBlock++;
    }

    return true;
}

void* PHYSMEM_AllocBlock()
{
    uint32_t block = PHYSMEM_firstFreeBlock();

    if(block == -1)
        return NULL;
    
    PHYSMEM_setBlockToUsed(block);
    PHYSMEM_totalUsedBlock++;
    PHYSMEM_totalFreeBlock--;

    return (void*)(block * BLOCK_SIZEKB * 0x400);
}

void* PHYSMEM_AllocBlocks(uint8_t block_size)
{
    uint32_t index;
    uint32_t block_addr;
    uint8_t count;

    if(block_size > PHYSMEM_totalFreeBlock)
        return NULL;
    
    index = PHYSMEM_firstFreeBlock();
    block_addr = index;
    count = 1; // we already have one block

    while (index < PHYSMEM_totalBlockNumber)
    {
        if(count >= block_size)
        {
            for(int i = 0; i < count; i++)
            {
                PHYSMEM_setBlockToUsed(block_addr + i);
                PHYSMEM_totalUsedBlock++;
                PHYSMEM_totalFreeBlock--;
            }
            
            return (void*)(block_addr * BLOCK_SIZEKB * 0x400);
        }

        index++;
        if(PHYSMEM_checkIfBlockUsed(index))
        {
            count = 1;
            index = PHYSMEM_firstFreeBlockFrom(index);
            block_addr = index;
        }

        count++;
    }
    
    return NULL;
}

void PHYSMEM_freeBlock(void* ptr)
{
    if(!ptr)
        return;

    uint32_t block = (uint32_t)ptr / (BLOCK_SIZEKB * 0x400);

    PHYSMEM_setBlockToFree(block);
    PHYSMEM_totalUsedBlock--;
    PHYSMEM_totalFreeBlock++;
}

void PHYSMEM_freeBlocks(void* ptr, uint8_t size)
{
    if(!ptr)
        return;

    uint32_t block = (uint32_t)ptr / (BLOCK_SIZEKB * 0x400);

    for(int i = 0; i < size; i++)
    {
        PHYSMEM_setBlockToFree(block + i);
        PHYSMEM_totalUsedBlock--;
        PHYSMEM_totalFreeBlock++;
    }
}