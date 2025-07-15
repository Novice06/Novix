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

#include <hal/idt.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

typedef struct{
    uint16_t offset_low;    // offset (bit 0-15)
    uint16_t segment;       // segment
    uint8_t reserved;       // reserved
    uint8_t attribute;      // gate type, dpl, and p fields
    uint16_t offset_high;   // offset (bit 16-31)
}__attribute__((packed)) Idt_gate;

typedef struct{
    uint16_t size;
    Idt_gate *offset;
}__attribute__((packed)) Idt_descriptor;

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

Idt_gate g_IDT[256];
Idt_descriptor g_IDTdescriptor = {sizeof(g_IDT) - 1, g_IDT};

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================

void __attribute__((cdecl)) IDT_flush(Idt_descriptor* descriptor);

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void IDT_setGate(int interrupt, void* offset, uint8_t attribute)
{
    g_IDT[interrupt].offset_low     = (uint32_t)offset & 0XFFFF;
    g_IDT[interrupt].segment        = 0X08; // code segment
    g_IDT[interrupt].reserved       = 0;
    g_IDT[interrupt].attribute      = attribute;
    g_IDT[interrupt].offset_high    = ((uint32_t)offset >> 16) & 0xFFFF;
}

void IDT_initilize()
{
    IDT_flush(&g_IDTdescriptor);
}