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

#define MAX_MEMORY_ENTRY 256
#include "x86.h"
#include "memdefs.h"
#include "stdio.h"

Memory_mapEntry* g_memoryBlockEntries = MEMORY_MAP_ADDR;

void memoryDetect(Boot_info* info)
{
    Memory_mapEntry memoryBlockEntry;

    uint32_t continuation = 0;
    uint32_t memoryBlockCount = 0;
    uint16_t ret = 0;

    info->memorySize = x86_Get_MemorySize();

    if(x86_Get_MemoryMap(g_memoryBlockEntries, &memoryBlockCount) == -1)
        return; // failed

    info->memoryBlockCount = memoryBlockCount;
    info->memoryBlockEntries = g_memoryBlockEntries;
}