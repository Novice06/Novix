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
#include <stddef.h>

typedef struct fat_dir_entry
{
    char filename[11];            // File name (8 characters) and File extension (3 characters)
    uint8_t attributes;          // File attributes (read-only, hidden, system, etc.)
    uint8_t reserved;            // Reserved (usually zero)
    uint8_t creationTimeTenth;   // Creation time in tenths of a second
    uint16_t creationTime;       // Creation time (HHMMSS format)
    uint16_t creationDate;       // Creation date (YYMMDD format)
    uint16_t lastAccessDate;     // Last access date
    uint16_t firstClusterHigh;   // High word of the first cluster number (FAT32 only)
    uint16_t writeTime;          // Last write time
    uint16_t writeDate;          // Last write date
    uint16_t firstClusterLow;    // Low word of the first cluster number
    uint32_t fileSize;           // File size in bytes
} __attribute__((packed)) fat_dir_entry_t;

typedef struct file
{
    fat_dir_entry_t* entry;
    uint32_t readPos;
}file_t;

typedef void (*readDiskSectors)(void* buffer, uint32_t lba, uint32_t sector_num);

void fat32_init(readDiskSectors read);
file_t* fat32_open(const char* path);
int fat32_read(file_t* this_file, void* buffer, size_t size);