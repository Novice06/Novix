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
    uint64_t base;
    uint64_t length;
    uint16_t type;
    uint16_t acpi;
}Memory_mapEntry;

typedef enum{
	AVAILABLE 	= 1,
	RESERVED 	= 2,
	ACPI 		= 3,
	ACPI_NVS 	= 4,
}MEMORY_TYPE;

typedef struct video_info
{
    uint16_t pitch;			    // number of bytes per horizontal line
	uint16_t width;			    // width in pixels
    uint16_t height;			// height in pixels
    uint8_t bpp;			    // bits per pixel in this mode
    uint8_t bytes_per_pixel;

    uint8_t memory_model;

    uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;

    uint32_t framebuffer;		    // physical address of the linear frame buffer; write here to draw to the screen
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
}__attribute__((packed)) video_info_t;


typedef struct{
    uint16_t bootDrive;
    uint32_t memorySize;
    uint32_t memoryBlockCount;
    Memory_mapEntry* memoryBlockEntries;
    video_info_t video_info;
}Boot_info;