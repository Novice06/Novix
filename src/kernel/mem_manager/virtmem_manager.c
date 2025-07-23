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
#include <stddef.h>
#include <mem_manager/physmem_manager.h>
#include <mem_manager/virtmem_manager.h>
#include <memory.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define PAGE_ADD_ATTRIBUTE(page_entry, flags)     page_entry | flags
#define PAGE_SET_FRAME(page_entry, frame)         page_entry | frame

#define PTE_INDEX(virt_addr) (virt_addr >> 12) & 0x3ff
#define PDE_INDEX(virt_addr) (virt_addr >> 22) & 0x3ff

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

bool VIRTMEM_allocPage(PTE* entry, uint32_t flags)
{
    PTE page = (PTE)PHYSMEM_AllocBlock(); // allocate a free physical frame
    if(!page)
        return false;

    *entry = PAGE_ADD_ATTRIBUTE(page, flags);

    return true;
}

void VIRTMEM_freePage(PTE* entry)
{
    void* ptr = (void*)(*entry & 0xFFFFF000);
    PHYSMEM_freeBlock(ptr);

    *entry = 0x0; // page not present
}

bool VIRTMEM_mapTable(void* virt, bool kernel_mode)
{
    if((uint32_t)virt >= 0xFFC00000) // arealdy used by recursive mapping
        return false;

    PDE* page_directory = (PDE*)0xFFFFF000; // virtual addresse of the page directory
    
    uint32_t pageTableIndex = PDE_INDEX((uint32_t)virt);
    PTE* page_table = (PTE*)(0xFFC00000 + (pageTableIndex << 12));   // virtuall addresse of the page table

    if((page_directory[pageTableIndex] & PDE_PRESENT) != PDE_PRESENT)
    {
        void* frame = PHYSMEM_AllocBlock(); // physical address of the page table
        if(!frame)
            return false;
        
        if(kernel_mode)
            page_directory[pageTableIndex] = PAGE_ADD_ATTRIBUTE((uint32_t)frame, PDE_PRESENT | PDE_WRITE | PDE_KERNEL_MODE);
        else
            page_directory[pageTableIndex] = PAGE_ADD_ATTRIBUTE((uint32_t)frame, PDE_PRESENT | PDE_WRITE | PDE_USER_MODE);
            
        
        memset(page_table, 0, 0x1000);
    }

    return true;
}

bool VIRTMEM_unMapTable(void* virt)
{
    if((uint32_t)virt >= 0xFFC00000) // arealdy used by recursive mapping
        return false;

    PDE* page_directory = (PDE*)0xFFFFF000; // virtual addresse of the page directory
    
    uint32_t pageTableIndex = PDE_INDEX((uint32_t)virt);

    if((page_directory[pageTableIndex] & PDE_PRESENT) != PDE_PRESENT)
        return true;    // alredy unmapped


    void* frame = (void*)(page_directory[pageTableIndex] & 0xFFFFF000);
    PHYSMEM_freeBlock(frame);
    page_directory[pageTableIndex] = 0;

    return true;
}

bool VIRTMEM_mapPage (void* virt, bool kernel_mode)
{
    // in case this page table doesn't exist
    if(!VIRTMEM_mapTable(virt, kernel_mode))
        return false;

    PDE* page_directory = (PDE*)0xFFFFF000; // virtual addresse of the page directory

    uint32_t pageTableIndex = PDE_INDEX((uint32_t)virt);
    PTE* page_table = (PTE*)(0xFFC00000 + (pageTableIndex << 12));   // virtuall addresse of the page table

    uint32_t pageEntryIndex = PTE_INDEX((uint32_t)virt);
    if((page_table[pageEntryIndex] & PTE_PAGE_PRESENT) == PTE_PAGE_PRESENT)
        return true; // page already mapped nothing to do

    if(kernel_mode)
    {
        if(!VIRTMEM_allocPage(&page_table[pageEntryIndex], PTE_PAGE_PRESENT | PTE_PAGE_WRITE | PTE_PAGE_KERNEL_MODE)) // PTE_PAGE_PRESENT | PTE_PAGE_WRITE | PTE_PAGE_KERNEL_MODE
            return false;
    }
    else
    {
        if(!VIRTMEM_allocPage(&page_table[pageEntryIndex], PTE_PAGE_PRESENT | PTE_PAGE_WRITE | PTE_PAGE_USER_MODE))
            return false;
    }
        
    
    flushTLB(virt);
    return true;
}

bool VIRTMEM_unMapPage (void* virt)
{
    if((uint32_t)virt >= 0xFFC00000) // arealdy used by recursive mapping
        return false;

    PDE* page_directory = (PDE*)0xFFFFF000; // virtual addresse of the page directory
    
    uint32_t pageTableIndex = PDE_INDEX((uint32_t)virt);
    PTE* page_table = (PTE*)(0xFFC00000 + (pageTableIndex << 12));   // virtuall addresse of the page table

    if((page_directory[pageTableIndex] & PDE_PRESENT) != PDE_PRESENT)
        return true;    // already unmapped

    uint32_t pageEntryIndex = PTE_INDEX((uint32_t)virt);
    if((page_table[pageEntryIndex] & PTE_PAGE_PRESENT) != PTE_PAGE_PRESENT)
        return true; // page already unmapped nothing to do

    VIRTMEM_freePage(&page_table[pageEntryIndex]);
    
    flushTLB(virt);
    return true;
}

uint32_t* VIRTMEM_getPhysAddr(void* virt)
{   
    uint32_t pageTableIndex = PDE_INDEX((uint32_t)virt);
    uint32_t pageEntryIndex = PTE_INDEX((uint32_t)virt);

    PTE* page_table = (PTE*)(0xFFC00000 + (pageTableIndex << 12));   // virtuall addresse of the page table

    return (uint32_t*)(page_table[pageEntryIndex] & 0xFFFFF000);
}

bool VIRTMEM_initialize()
{
    // allocate default page directory table
    PDE* page_directory = PHYSMEM_AllocBlock();

    // allocate first page table
    PTE* table_0 = PHYSMEM_AllocBlock();

    // allocates 3gb page table
    PTE* table_768 = PHYSMEM_AllocBlock();

    if(page_directory == NULL || table_0 == NULL || table_768 == NULL)
        return false;

    // 1st 4mb are idenitity mapped
   for (int i=0, frame=0x0, virt=0x00000000; i<1024; i++, frame+=4096, virt+=4096)
   {

      // create a new page
      PTE page = 0;
      page = PAGE_ADD_ATTRIBUTE(page, PTE_PAGE_PRESENT | PTE_PAGE_WRITE | PTE_PAGE_KERNEL_MODE);
      page = PAGE_SET_FRAME(page, frame);

      // ...and add it to the page table
      table_0[PTE_INDEX(virt)] = page;
   }

   // map 1mb to 3gb (where we are at)
   for (int i=0, frame=0x100000, virt=0xc0000000; i<1024; i++, frame+=4096, virt+=4096)
   {

      // create a new page
      PTE page = 0;
      page = PAGE_ADD_ATTRIBUTE(page, PTE_PAGE_PRESENT | PTE_PAGE_WRITE | PTE_PAGE_KERNEL_MODE);
      page = PAGE_SET_FRAME(page, frame);

      // ...and add it to the page table
      table_768[PTE_INDEX(virt)] = page;
   }

    // clear and initialize directory table
    memset(page_directory, 0, 0x1000);
    page_directory[PDE_INDEX(0x0)] = PAGE_ADD_ATTRIBUTE((uint32_t)table_0, PDE_PRESENT | PDE_WRITE | PDE_KERNEL_MODE);
    page_directory[PDE_INDEX(0xc0000000)] = PAGE_ADD_ATTRIBUTE((uint32_t)table_768, PDE_PRESENT | PDE_WRITE | PDE_KERNEL_MODE);

    // recursive mapping here !
    page_directory[1023] = PAGE_ADD_ATTRIBUTE((uint32_t)page_directory, PDE_PRESENT | PDE_WRITE | PDE_KERNEL_MODE);

    switchPDBR(page_directory);
    enablePaging();    // just in case ...

    return true;
}