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

typedef enum{
    IDT_ATTRIBUTE_TASK_GATE             = 0x05,
    IDT_ATTRIBUTE_16BIT_INTERRUPT_GATE  = 0X06,
    IDT_ATTRIBUTE_16BIT_TRAP_GATE       = 0X07,
    IDT_ATTRIBUTE_32BIT_INTERRUPT_GATE  = 0X0E,
    IDT_ATTRIBUTE_32BIT_TRAP_GATE       = 0X0F,

    IDT_ATTRIBUTE_DPL_RING0             = 0X00,
    IDT_ATTRIBUTE_DPL_RING1             = 0X20,
    IDT_ATTRIBUTE_DPL_RING2             = 0X40,
    IDT_ATTRIBUTE_DPL_RING3             = 0X60,

    IDT_ATTRIBUTE_PRESENT_BIT           = 0X80,
}IDT_ATTRIBUTE_BYTE;

void i686_IDT_initilize();
void i686_IDT_setGate(int interrupt, void* offset, uint8_t attribute);