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

#include "fat32.h"

typedef struct fat_BS
{
	uint8_t bootjmp[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t table_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t table_size_16;
    uint16_t sectors_per_track;
    uint16_t head_side_count;
    uint32_t hidden_sector_count;
    uint32_t total_sectors_32;
	
	//extended fat32 stuff
	uint32_t table_size_32;
	uint16_t extended_flags;
	uint16_t fat_version;
	uint32_t root_cluster;
	uint16_t fat_info;
	uint16_t backup_BS_sector;
	uint8_t reserved_0[12];
	uint8_t drive_number;
	uint8_t reserved_1;
	uint8_t boot_signature;
	uint32_t volume_id;
	uint8_t volume_label[11];
	uint8_t fat_type_label[8];

    uint8_t Filler[422];            //needed to make struct 512
	
}__attribute__((packed)) fat_BS_t;

typedef enum
{
    FAT_ATTR_REGULAR    = 0x00, // Regular file with no special attribute
    FAT_ATTR_READ_ONLY  = 0x01, // File is read-only
    FAT_ATTR_HIDDEN     = 0x02, // File is hidden
    FAT_ATTR_SYSTEM     = 0x04, // System file
    FAT_ATTR_VOLUME_ID  = 0x08, // Volume label
    FAT_ATTR_DIRECTORY  = 0x10, // Entry is a directory
    FAT_ATTR_ARCHIVE    = 0x20, // File should be archived
    FAT_ATTR_LFN        = 0x0F  // Long file name entry (special combination)
} fat_attributes_t;

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

typedef struct location
{
    uint32_t cluster; // where this entry is stored in its parent
    uint32_t offset;  // offset within the sector of its parent

    bool has_lfn_entries;
    uint32_t lfn_start_cluster;
    uint32_t lfn_offset;
    uint32_t lfn_entry_count;
}location_t;

typedef struct file
{
    fat_dir_entry_t entry;
    location_t in_parent;
    bool isRoot;
}file_t;

typedef enum {FILE_TYPE, DIR_TYPE}file_type;
typedef struct dirEntry
{
    char name[257]; // max 256 zero terminated fat32 filename
    file_type type;
}dirEntry_t;

typedef struct fat32_info
{
    vnode_t* total_vnode[MAX_VNODE_PER_VFS];
    vnode_t* root_vnode;
    fat_BS_t bootSector;
    uint32_t* FAT;
    void* working_buffer;

    device_t* dev;
}fat32_info_t;

int64_t fat32_read(fat32_info_t* filesystem, file_t* this_file, void* buffer, size_t size, uint64_t readPos);
int64_t fat32_write(fat32_info_t* filesystem, file_t* this_file, const void* buffer, size_t size, uint64_t readPos);

bool fat32_lookup_in_dir(fat32_info_t* filesystem, uint32_t first_cluster, const char* name, fat_dir_entry_t* entryOut, location_t* out);
int fat32_readdir(fat32_info_t* filesystem, file_t* this_dir, dirEntry_t* out, uint64_t readPos);