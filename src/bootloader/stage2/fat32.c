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

#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <stdbool.h>

#include "stdio.h"
#include "fat32.h"
#include "disk.h"
#include "memdefs.h"

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

typedef struct {
    uint8_t  order;          // 0x01, 0x02,..., 0x40 | N
    uint16_t name1[5];       // 5 chars
    uint8_t  attr;           // 0x0F
    uint8_t  type;           // 0x00
    uint8_t  checksum;       // checksum of 8.3 name
    uint16_t name2[6];       // 6 chars
    uint16_t firstCluster;   // 0x0000
    uint16_t name3[2];       // 2 chars
}__attribute__((packed)) LFN;


fat_BS_t bootSector;
uint32_t* FAT;
void* working_buffer;

struct
{
    file_t file;
    bool inuse;
} opened_files[32];
struct
{
    fat_dir_entry_t  entry;
    bool inuse;
}entry_buff[32];

readDiskSectors readSectors;

void fat32_init(readDiskSectors read)
{
    readSectors = read;
    readSectors(&bootSector, 0, 1);

    // print_buffer("fat32bootsect: ", &bootSector, sizeof(bootSector));

    FAT = (void*)MEMORY_FAT_ADDR;
    working_buffer = (void*)MEMORY_FATBUFFER_ADDR;

    readSectors(FAT, bootSector.reserved_sector_count, bootSector.table_size_32);
}

file_t* request_file_struc()
{
    for(int i = 0; i < 32; i++)
    {
        if(!opened_files[i].inuse)
        {
            opened_files[i].inuse = true;
            return &opened_files[i].file;
        }

    }

    return NULL;
}

void free_file_struc(file_t* file)
{
    for(int i = 0; i < 32; i++)
    {
        if(&opened_files[i].file == file)
        {
            opened_files[i].inuse = false;
            return;
        }

    }
}

fat_dir_entry_t* request_fatDirEntry_struc()
{
     for(int i = 0; i < 32; i++)
    {
        if(!entry_buff[i].inuse)
        {
            entry_buff[i].inuse = true;
            return &entry_buff[i].entry;
        }

    }

    return NULL;
}

void free_fatDirEntry_struc(fat_dir_entry_t* entry)
{
     for(int i = 0; i < 32; i++)
    {
        if(&entry_buff[i].entry == entry)
        {
            entry_buff[i].inuse = false;
            return;
        }

    }
}

uint32_t get_next_cluster(uint32_t currentCluster)
{    
    return FAT[currentCluster] & 0x0FFFFFFF;
}

uint32_t cluster_to_Lba(uint32_t cluster)
{
    uint16_t fat_total_size = (bootSector.table_size_32 * bootSector.table_count);

    return (bootSector.reserved_sector_count + fat_total_size) + (cluster - 2) * bootSector.sectors_per_cluster;
}

void string_to_fatname(const char* name, char* nameOut)
{
    char fatName[12];

    // convert from name to fat name
    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    const char* ext = strchr(name, '.');
    if (ext == NULL)
        ext = name + 11;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        fatName[i] = toupper(name[i]);

    if (ext != name + 11)
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            fatName[i + 8] = toupper(ext[i + 1]);

    memcpy(nameOut, fatName, 12);
}

uint8_t CalculateLFNChecksum(char *fatName)
{
    uint8_t sum = 0;

    for (int i = 11; i > 0; i--) {
        // 1. Circular Shift Right by 1 bit
        // We take the bit that "falls out" on the right (sum & 1)
        // and put it back on the far left (bit 7, which is 0x80)
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1);
        
        // 2. Add the current character's ASCII value
        sum += *fatName++;
    }

    return sum;
}
/* apprently C doesn't have insensitve case comparison */
int strCasecmp(const char *str1, const char *str2)
{
    while (toupper(*str1) == toupper(*str2))
    {
        if (*str1 == '\0')
            return 0;

        str1++;
        str2++;
    }

    return (*str1 < *str2) ? -1 : 1;
}

fat_dir_entry_t* fat32_lookup_in_dir(uint32_t first_cluster, char* name)
{
    fat_dir_entry_t* entry = NULL;

    bool found = false;

    enum {NORMAL_ENTRY, LFN_ENTRY};
    int entry_state = NORMAL_ENTRY;

    uint8_t order;
    uint8_t checksum;
    char lfn_name[256];

    uint32_t current_cluster = first_cluster;

    do  // this loop is used to trace the cluster chain of the directory
    {
        readSectors(working_buffer, cluster_to_Lba(current_cluster), bootSector.sectors_per_cluster);
        

        fat_dir_entry_t* current_entry;
        int EntryCount = (bootSector.sectors_per_cluster * bootSector.bytes_per_sector) / 32;

        for(int i = 0; i < EntryCount; i++) // this loop is used to parse the entries of a directory
        {
            current_entry = (fat_dir_entry_t*)working_buffer + i;

            switch (entry_state)
            {
            case NORMAL_ENTRY:
            _NORMAL_ENTRY:
                if((current_entry->attributes & FAT_ATTR_LFN) == FAT_ATTR_LFN)
                {
                    if(!(((LFN*)current_entry)->order & 0x40))
                    {
                        break; // ignore this entry because it's not the last and it's probably a corrupted LFN
                    }

                    entry_state = LFN_ENTRY;

                    memset(lfn_name, 0, 256);
                    order = ((LFN*)current_entry)->order & 0x3f;
                    checksum = ((LFN*)current_entry)->checksum;

                    // assembling the name
                    uint16_t name_13[13];
                    memcpy(name_13, ((LFN*)current_entry)->name1, sizeof(uint16_t) * 5);
                    memcpy(name_13 + 5, ((LFN*)current_entry)->name2, sizeof(uint16_t) * 6);
                    memcpy(name_13 + 11, ((LFN*)current_entry)->name3, sizeof(uint16_t) * 2);

                    // now copying the name into a uint8_t size
                    uint8_t offset = (order - 1) * 13;  // because each entry holds 13chars and depending on the order, we can calculate the starting offset of the word
                    for(int i = 0; i < 13; i++)
                        lfn_name[offset + i] = (char)name_13[i];;
                }
                else
                {
                    char fatname[12];
                    string_to_fatname(name, fatname);

                    if(strncmp(current_entry->filename, fatname, 11) == 0)
                    {
                        found = true;
                        entry = request_fatDirEntry_struc();

                        memcpy(entry, current_entry, sizeof(fat_dir_entry_t));
                        break;
                    }
                }

            break;
            
            case LFN_ENTRY:
                order--;

                if((order == 0) && ((current_entry->attributes & FAT_ATTR_LFN) == FAT_ATTR_LFN))
                {
                    entry_state = NORMAL_ENTRY; // corrupted LFN ignore
                }
                else if ((order != 0) && ((current_entry->attributes & FAT_ATTR_LFN) != FAT_ATTR_LFN))
                {
                    entry_state = NORMAL_ENTRY; // corrupted LFN ignore
                    goto _NORMAL_ENTRY; // the LFN entries are corrupted but we still need to treat this as a normal entry
                }
                else
                {
                    if(order == 0)
                    {
                        entry_state = NORMAL_ENTRY; // after this we need to go to the normal state

                        if(checksum != CalculateLFNChecksum(current_entry->filename))
                        {
                            // corrupted LFN ignore
                            goto _NORMAL_ENTRY; // the LFN entries are corrupted but we still need to treat this as a normal entry
                        }
                        
                        if(strCasecmp(lfn_name, name) == 0)
                        {
                            found = true;
                            entry = request_fatDirEntry_struc();

                            memcpy(entry, current_entry, sizeof(fat_dir_entry_t));
                            break;
                        }

                        break;  // exiting the switch statement
                    }

                    if(((LFN*)current_entry)->order != order)
                    {
                        entry_state = NORMAL_ENTRY; // corrupted LFN ignore
                        break; // no need to continue exiting this switch statement
                    }

                    // assembling the name
                    uint16_t name_13[13];
                    memcpy(name_13, ((LFN*)current_entry)->name1, sizeof(uint16_t) * 5);
                    memcpy(name_13 + 5, ((LFN*)current_entry)->name2, sizeof(uint16_t) * 6);
                    memcpy(name_13 + 11, ((LFN*)current_entry)->name3, sizeof(uint16_t) * 2);

                    // now copying the name into a uint8_t size
                    uint8_t offset = (order - 1) * 13;  // because each entry holds 13chars and depending on the order, we can calculate the starting offset of the word
                    for(int i = 0; i < 13; i++)
                        lfn_name[offset + i] = (char)name_13[i];

                }
            break;

            }
            
        }

        current_cluster = get_next_cluster(current_cluster);
        
    } while (current_cluster < 0x0FFFFFF8 && !found);

    return entry;
}

file_t* fat32_open(const char* path)
{
    fat_dir_entry_t* entry;

    char* name;

    char parsed_path[1024] = {0};
    strncpy(parsed_path, path, 1024);

    // searching in the root dir first
    name = strtok(parsed_path, "/");
    entry = fat32_lookup_in_dir(bootSector.root_cluster, name);

    if(!entry)
        return NULL;    // not found

    // now searching in the directories of the path
    name = strtok(NULL, "/");
    while (name != NULL && entry != NULL)
    {
        uint32_t first_cluster = entry->firstClusterLow | (entry->firstClusterHigh << 16);
        free_fatDirEntry_struc(entry);
        
        entry = fat32_lookup_in_dir(first_cluster, name);

        if(!entry)
            return NULL;    // not found

        name = strtok(NULL, "/");
    }

    file_t* this_file = request_file_struc();
    this_file->entry = entry;
    this_file->readPos = 0;
    
    return this_file;
}

int fat32_read(file_t* this_file, void* buffer, size_t size)
{
    if((this_file->entry->attributes & FAT_ATTR_REGULAR) != FAT_ATTR_REGULAR)
    {
        return -1;   // this isn't a file
    }

    if (this_file->readPos >= this_file->entry->fileSize)
        return -1;   // EOF

    // ajust the size to read !
    size = ((this_file->readPos + size) > this_file->entry->fileSize) ? (this_file->entry->fileSize - this_file->readPos) : size;

    uint32_t current_cluster = this_file->entry->firstClusterLow | (this_file->entry->firstClusterHigh << 16);
    uint32_t skipped_clusters = (uint32_t)(this_file->readPos / (bootSector.sectors_per_cluster * bootSector.bytes_per_sector));

    uint32_t f = current_cluster;
    
    /* Some clusters are skipped since, based on the read position, their content doesn't need to be read. */
    for(int i = 0; i < skipped_clusters; i++)
        current_cluster = get_next_cluster(current_cluster);

    /* This is an offset based on the cluster currently being read, according to the current read position */
    uint32_t offset = this_file->readPos - (skipped_clusters * bootSector.sectors_per_cluster * bootSector.bytes_per_sector);
    size_t to_read = 0; // to keep track of how many byte we've read
    while (current_cluster < 0x0FFFFFF8 && to_read < size)
    {
        readSectors(working_buffer, cluster_to_Lba(current_cluster), bootSector.sectors_per_cluster);

        /* "Bytes to read, to ensure we don’t exceed the size of the data in the buffer. */
        uint16_t byte_to_read = (bootSector.sectors_per_cluster * bootSector.bytes_per_sector) - offset;
        byte_to_read = ((byte_to_read + to_read) > size) ? (size - to_read) : byte_to_read; // ajust the byte to read based on the actual size to read !

        print_buffer("kernel: lba: %d", (void*)working_buffer, 512);
        printf("lba: %d, firstC %d, flba: %d\n", cluster_to_Lba(current_cluster), f, cluster_to_Lba(f));

        memcpy(buffer + to_read, working_buffer + offset, byte_to_read);

        to_read += byte_to_read;    // increase the number of byte read
        offset = 0;    // the offset reset to 0 for the next cluster !
        current_cluster = get_next_cluster(current_cluster);
    }
    
    return to_read; // return the number of byte read !
}