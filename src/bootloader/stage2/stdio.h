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
#include "stdint.h"

#define COLOR8_BLACK 0
#define COLOR8_LIGHT_GREY 7

#define WIDTH 80
#define HEIGHT 25

void puts(const char* s);
void putc(const char s);
void printf(const char* fmt, ...);
void print_buffer(const char* msg, const void* buffer, uint32_t count);
void scrollUp();
void newLine();
void clr();
void updateCursor();
void debugs(const char* str);
void E9_putc(char c);
void debugs(const char* str);
