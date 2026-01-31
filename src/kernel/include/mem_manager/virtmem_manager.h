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

typedef enum {
    PTE_PAGE_PRESENT        = 0X1,
    PTE_PAGE_WRITE          = 0X2,
    PTE_PAGE_KERNEL_MODE    = 0X0,
    PTE_PAGE_USER_MODE      = 0X4,
}PTE_FLAGS;

typedef enum {
    PDE_PRESENT             = 0X01,
    PDE_WRITE               = 0X02,
    PDE_KERNEL_MODE         = 0X00,
    PDE_USER_MODE           = 0X04,
    PDE_PWT                 = 0X08,
    PDE_PCD                 = 0X10,
    PDE_4KBPAGE             = 0X00,
    PDE_4MBPAGE             = 0X80,
}PDE_FLAGS;

//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================

bool VIRTMEM_initialize(uint32_t kernel_size);
uint32_t* VIRTMEM_getPhysAddr(void* virt);

bool VIRTMEM_mapTable(void* virt, bool kernel_mode);
bool VIRTMEM_unMapTable(void* virt);

bool VIRTMEM_mapPage (void* virt, bool kernel_mode);
bool VIRTMEM_unMapPage (void* virt);

bool VIRTMEM_mapPageCustom (void* phys, void* virt, bool kernel_mode);
void* VIRTMEM_unMapPageCustom (void* virt);

void VIRTMEM_freePage(PTE* entry);
bool VIRTMEM_allocPage(PTE* entry, uint32_t flags);

uint32_t* VIRTMEM_createAddressSpace();
void VIRTMEM_destroyAddressSpace(PDE* page_directory);

void __attribute__((cdecl)) enablePaging();
void __attribute__((cdecl)) flushTLB(uint32_t* virtual_addr);
void* __attribute__((cdecl)) getPDBR();
void __attribute__((cdecl)) switchPDBR(uint32_t* physical_addr);