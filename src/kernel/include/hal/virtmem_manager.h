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


#pragma once
#include <stdint.h>
#include <stdbool.h>

//============================================================================
//    INTERFACE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

typedef uint32_t PDE;
typedef uint32_t PTE;

//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================

void VIRTMEM_initialize();
bool VIRTMEM_mapPage (void* virt);
void VIRTMEM_freePage(PTE* entry);
bool VIRTMEM_allocPage(PTE* entry, uint32_t flags);