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
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================

void __attribute__((cdecl)) outb(uint16_t port, uint8_t value);
uint8_t __attribute__((cdecl)) inb(uint16_t port);
void __attribute__((cdecl)) panic();
void __attribute__((cdecl)) HLT();
void __attribute__((cdecl)) enableInterrupts();
void __attribute__((cdecl)) disableInterrupts();
uint32_t __attribute__((cdecl)) get_eflags();