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

#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <drivers/device.h>
#include <debug.h>
#include <stdio.h>

#include "fat32.h"
#include "fat32_interface.h"

#define IS_VALID_CLUSTER(c) (c > 1 && c < 0x0FFFFFF8)

static inline void writeSectors(device_t* dev, const void* buffer, uint32_t lba, uint32_t sector_num)
{
    dev->write(buffer, lba, sector_num, dev->priv, 0);
}
static inline void readSectors(device_t* dev, void* buffer, uint32_t lba, uint32_t sector_num)
{
    dev->read(buffer, lba, sector_num, dev->priv, 0);
}

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

typedef struct dir_iterator
{
    uint32_t current_cluster;
    uint32_t offset;
}dir_iterator_t;

// uint16_t fat32_encode_time(struct tm* t)
// {
//     /*
//     * FAT32 Time format (16 bits):
//     *  Bits 15-11 : Hours   (0-23)
//     *  Bits 10-5  : Minutes (0-59)
//     *  Bits 4-0   : Seconds / 2 (0-29)
//     */

//     return ((t->tm_hour & 0x1F) << 11) |
//            ((t->tm_min  & 0x3F) << 5)  |
//            ((t->tm_sec / 2)     & 0x1F);
// }

// uint16_t fat32_encode_date(struct tm* t)
// {
//     /*
//     * FAT32 Date format (16 bits):
//     *  Bits 15-9 : Year depuis 1980 (0-127 → 1980-2107)
//     *  Bits 8-5  : Month (1-12)
//     *  Bits 4-0  : Day   (1-31)
//     */

//     return (((t->tm_year + 1900 - 1980) & 0x7F) << 9) |
//            (((t->tm_mon + 1)            & 0x0F) << 5) |
//            ((t->tm_mday)                & 0x1F);
// }

// uint8_t fat32_encode_tenths(struct tm* t)
// {
//     uint8_t centiseconds = 0;
//     return (t->tm_sec % 2) ? (100 + centiseconds) : centiseconds;
// }

// void fat32_set_timestamps(fat_dir_entry_t* entry)
// {
//     time_t now = time(NULL);
//     struct tm* t = localtime(&now);

//     uint16_t date = fat32_encode_date(t);
//     uint16_t time = fat32_encode_time(t);
//     uint8_t  tenth = fat32_encode_tenths(t);

//     entry->creationDate      = date;
//     entry->creationTime      = time;
//     entry->creationTimeTenth = tenth;
//     entry->lastAccessDate    = date;
//     entry->writeDate         = date;
//     entry->writeTime         = time;
// }

uint32_t get_next_cluster(fat32_info_t* filesystem, uint32_t currentCluster)
{    
    uint32_t fat_sector = (currentCluster * 4) / filesystem->bootSector.bytes_per_sector;   // the sector where this cluster is stored
    int clusters_per_sector = filesystem->bootSector.bytes_per_sector / 4;

    readSectors(filesystem->dev, filesystem->FAT, filesystem->bootSector.reserved_sector_count + fat_sector, 1);

    uint32_t index_in_sector = currentCluster % clusters_per_sector;

    return filesystem->FAT[index_in_sector] & 0x0FFFFFFF;
}

uint32_t cluster_to_Lba(fat32_info_t* filesystem, uint32_t cluster)
{
    uint16_t fat_total_size = (filesystem->bootSector.table_size_32 * filesystem->bootSector.table_count);

    return (filesystem->bootSector.reserved_sector_count + fat_total_size) + (cluster - 2) * filesystem->bootSector.sectors_per_cluster;
}

void free_cluster_chain(fat32_info_t* filesystem, uint32_t cluster)
{
    uint32_t fat_sector = (cluster * 4) / filesystem->bootSector.bytes_per_sector;   // the sector where this cluster is stored
    int clusters_per_sector = filesystem->bootSector.bytes_per_sector / 4;
    uint32_t index_in_sector = cluster % clusters_per_sector;

    uint32_t next_cluster = get_next_cluster(filesystem, cluster);

    if(IS_VALID_CLUSTER(next_cluster))
        free_cluster_chain(filesystem, next_cluster);    // let the power of recursion do its thing !!

    readSectors(filesystem->dev, filesystem->FAT, filesystem->bootSector.reserved_sector_count + fat_sector, 1);
    filesystem->FAT[index_in_sector] = 0x00000000;
    writeSectors(filesystem->dev, filesystem->FAT, filesystem->bootSector.reserved_sector_count + fat_sector, 1);    // flush
}

uint32_t alloc_newCluster(fat32_info_t* filesystem, uint32_t last_cluster)
{
    int clusters_per_sector = filesystem->bootSector.bytes_per_sector / 4;

    for(int i = 0; i < filesystem->bootSector.table_size_32; i++)   // load the fat by chunk
    {
        readSectors(filesystem->dev, filesystem->FAT, filesystem->bootSector.reserved_sector_count + i, 1);

        for(int j = 0; j < clusters_per_sector; j++)
        {
            if(filesystem->FAT[j] == 0) // bingo its free
            {
                filesystem->FAT[j] = 0x0FFFFFF8; // now this cluster is in use

                writeSectors(filesystem->dev, filesystem->FAT, filesystem->bootSector.reserved_sector_count + i, 1); // flush

                if(IS_VALID_CLUSTER(last_cluster))
                {
                    uint32_t fat_sector = (last_cluster * 4) / filesystem->bootSector.bytes_per_sector;   // the sector where this cluster is stored
                    readSectors(filesystem->dev, filesystem->FAT, filesystem->bootSector.reserved_sector_count + fat_sector, 1);

                    uint32_t index_in_sector = last_cluster % clusters_per_sector;
                    filesystem->FAT[index_in_sector] = i * clusters_per_sector + j;         // put the new cluster to the cluster chain

                    writeSectors(filesystem->dev, filesystem->FAT, filesystem->bootSector.reserved_sector_count + fat_sector, 1); // flush
                }
                
                return i * clusters_per_sector + j;
            }
        }
    }

    return 0;   // not found !!
}

int is_valid_83_char(char c) {
    return (
        isalnum(c) ||
        c == '$' || c == '%' || c == '\'' || c == '-' ||
        c == '_' || c == '@' || c == '~' || c == '`' ||
        c == '!' || c == '(' || c == ')' || c == '{' ||
        c == '}' || c == '^' || c == '#' || c == '&'
    );
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

    for (int i = 0, char_8 = 0; char_8 < 8 && name[i] && name + i < ext; i++)
    {
        if(is_valid_83_char(name[i]))
            fatName[char_8] = toupper(name[i]);
        else
            char_8 = char_8 == 0 ? char_8 : char_8 - 1;

        char_8++;
    }

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

void fat32_sync_entry(fat32_info_t* filesystem, file_t* this_file)
{
    if(this_file->isRoot)   return; // what the fuck ??

    readSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, this_file->in_parent.cluster), filesystem->bootSector.sectors_per_cluster);
    fat_dir_entry_t* current_entry = (fat_dir_entry_t*)filesystem->working_buffer + this_file->in_parent.offset;

    memcpy(current_entry, &this_file->entry, sizeof(fat_dir_entry_t));
    writeSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, this_file->in_parent.cluster), filesystem->bootSector.sectors_per_cluster);  // flush !!
}

int64_t fat32_read(fat32_info_t* filesystem, file_t* this_file, void* buffer, size_t size, uint64_t readPos)
{
    if((this_file->entry.attributes & FAT_ATTR_REGULAR) != FAT_ATTR_REGULAR)
    {
        return -1;   // this isn't a file
    }

    if (readPos >= this_file->entry.fileSize)
        return 0;   // EOF

    // ajust the size to read !
    size = ((readPos + size) > this_file->entry.fileSize) ? (this_file->entry.fileSize - readPos) : size;

    uint32_t current_cluster = this_file->entry.firstClusterLow | (this_file->entry.firstClusterHigh << 16);
    uint32_t skipped_clusters = (uint32_t)(readPos / (filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector));
    
    /* Some clusters are skipped since, based on the read position, their content doesn't need to be read. */
    for(int i = 0; i < skipped_clusters; i++)
        current_cluster = get_next_cluster(filesystem, current_cluster);

    /* This is an offset based on the cluster currently being read, according to the current read position */
    uint32_t offset = readPos - (skipped_clusters * filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector);
    size_t to_read = 0; // to keep track of how many byte we've read
    while (IS_VALID_CLUSTER(current_cluster) && to_read < size)
    {
        readSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, current_cluster), filesystem->bootSector.sectors_per_cluster);

        /* "Bytes to read, to ensure we don’t exceed the size of the data in the buffer. */
        uint16_t byte_to_read = (filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector) - offset;
        byte_to_read = ((byte_to_read + to_read) > size) ? (size - to_read) : byte_to_read; // ajust the byte to read based on the actual size to read !

        memcpy(buffer + to_read, filesystem->working_buffer + offset, byte_to_read);

        to_read += byte_to_read;    // increase the number of byte read
        offset = 0;    // the offset reset to 0 for the next cluster !
        current_cluster = get_next_cluster(filesystem, current_cluster);
    }

    // time_t now = time(NULL);
    // struct tm* t = localtime(&now);
    // uint16_t date = fat32_encode_date(t);
    // this_file->entry.lastAccessDate = date;
    // fat32_sync_entry(this_file);

    return to_read; // return the number of byte read !
}

int64_t fat32_write(fat32_info_t* filesystem, file_t* this_file, const void* buffer, size_t size, uint64_t readPos)
{
    if((this_file->entry.attributes & FAT_ATTR_REGULAR) != FAT_ATTR_REGULAR)
    {
        return -1;   // this isn't a file
    }

    uint32_t last_cluster;   // used to keep track of the last sector before getting 0x0FFFFFF8

    uint32_t current_cluster = this_file->entry.firstClusterLow | (this_file->entry.firstClusterHigh << 16);
    if(!IS_VALID_CLUSTER(current_cluster))
    {
        uint32_t new_cluster = alloc_newCluster(filesystem, 0);
        if(!IS_VALID_CLUSTER(new_cluster)) return false;

        this_file->entry.firstClusterLow = new_cluster;
        this_file->entry.firstClusterHigh = new_cluster >> 16;

        fat32_sync_entry(filesystem, this_file);
        current_cluster = new_cluster;  // update current cluster
    }

    uint32_t skipped_clusters = (uint32_t)(readPos / (filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector));

     for(int i = 0; i < skipped_clusters; i++)
        current_cluster = get_next_cluster(filesystem, current_cluster);

    /* This is an offset based on the cluster currently being write, according to the current write position */
    uint32_t offset = readPos - (skipped_clusters * filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector);
    size_t to_write = 0; // to keep track of how many byte we've write

    while (IS_VALID_CLUSTER(current_cluster) && to_write < size)
    {
        last_cluster = current_cluster;

        readSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, current_cluster), filesystem->bootSector.sectors_per_cluster);

        uint16_t byte_to_write = (filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector) - offset;
        byte_to_write = ((byte_to_write + to_write) > size) ? (size - to_write) : byte_to_write;

        memcpy(filesystem->working_buffer + offset, buffer + to_write, byte_to_write);

        writeSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, current_cluster), filesystem->bootSector.sectors_per_cluster); // flush !!

        to_write += byte_to_write;    // increase the number of byte writeen

        offset = 0;    // the offset reset to 0 for the next cluster !
        current_cluster = get_next_cluster(filesystem, current_cluster);
    }

    while(to_write < size) // we need to allocate new cluworking_buffersters
    {
        last_cluster = alloc_newCluster(filesystem, last_cluster);
        if(!last_cluster)
        {
            goto End;   // still need to update (so the user doesnt lose info)
            /* THERE YOU SHOULD THROW AN ERROR TO INDICATE THAT THE DISK IS FULL*/
        }

        uint16_t byte_to_write = (filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector);
        byte_to_write = ((byte_to_write + to_write) > size) ? (size - to_write) : byte_to_write;

        memcpy(filesystem->working_buffer + offset, buffer + to_write, byte_to_write);

        writeSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, last_cluster), filesystem->bootSector.sectors_per_cluster); // flush !!

        to_write += byte_to_write;    // increase the number of byte writen
    }

End:
    this_file->entry.fileSize = (readPos + to_write) > this_file->entry.fileSize ? (readPos + to_write) : this_file->entry.fileSize;
    
    // time_t now = time(NULL);
    // struct tm* t = localtime(&now);
    // uint16_t date = fat32_encode_date(t);
    // this_file->entry.lastAccessDate = date;
    // this_file->entry.writeDate = date;
    // this_file->entry.writeTime = fat32_encode_time(t);;

    fat32_sync_entry(filesystem, this_file);

    return to_write;
}

int fat32_get_next_entry(fat32_info_t* filesystem, dir_iterator_t* dir, fat_dir_entry_t* entryOut, location_t* out, char* nameOut)
{
    enum {NORMAL_ENTRY, LFN_ENTRY};
    int entry_state = NORMAL_ENTRY;

    uint8_t order;
    uint8_t checksum;
    char lfn_name[256];
    int entries_consumed = 0;

    while (IS_VALID_CLUSTER(dir->current_cluster))  // this loop is used to trace the cluster chain of the directory
    {
        readSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, dir->current_cluster), filesystem->bootSector.sectors_per_cluster);

        fat_dir_entry_t* current_entry;
        int EntryCount = (filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector) / 32;

        for(int index = dir->offset; index < EntryCount; index++) // this loop is used to parse the entries of a directory
        {
            current_entry = (fat_dir_entry_t*)filesystem->working_buffer + index;
            entries_consumed++;

            if((uint8_t)current_entry->filename[0] == 0xE5)
            {
                entry_state = NORMAL_ENTRY;
                continue; // skip this entry because it has been deleted
            }

            switch (entry_state)
            {
            case NORMAL_ENTRY:
            _NORMAL_ENTRY:
                if(current_entry->filename[0] == 0)     return false;    // EOF

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
                        lfn_name[offset + i] = (char)name_13[i];

                    // initialize location for lfn entries
                    out->has_lfn_entries = true;
                    out->lfn_start_cluster = dir->current_cluster;
                    out->lfn_offset = index;
                    out->lfn_entry_count = 1;

                }
                else
                {
                    // set location
                    out->cluster = dir->current_cluster;
                    out->offset  = index;
                    out->has_lfn_entries = false;

                    if((current_entry->attributes & FAT_ATTR_DIRECTORY) == FAT_ATTR_DIRECTORY)
                    {
                        strncpy(nameOut, current_entry->filename, 11);
                        for(int i = 10; i >= 0 && isspace(nameOut[i]); i--) nameOut[i] = '\0';   // trim trailing spaces

                    }
                    else
                    {
                        memset(nameOut, 0, 257);
                        strncpy(nameOut, current_entry->filename, 8); // name
                        for(int i = 7; i >= 0 && isspace(nameOut[i]); i--) nameOut[i] = '\0';   // trim trailing spaces

                        if(!isspace(current_entry->filename[8]))    // if we have an extension
                        {
                            strcat(nameOut, ".");
                            strncat(nameOut, current_entry->filename+8, 3);   // extension
                        }
                    }

                    memcpy(entryOut, current_entry, sizeof(fat_dir_entry_t));

                    dir->offset = index + 1;    // VERY IMPORTANT BECAUSE WE WANT TO START READING THE NEXT ENTRY WHEN THIS FUNCTION IS CALLED AGAIN
                    return entries_consumed;            // because we found an entry !!
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

                        
                        // set location
                        out->cluster = dir->current_cluster;
                        out->offset  = index;
                        // all other fields have been set throughout the process
                    
                        strncpy(nameOut, lfn_name, 256);
                        memcpy(entryOut, current_entry, sizeof(fat_dir_entry_t));

                        dir->offset = index + 1;    // VERY IMPORTANT BECAUSE WE WANT TO START READING THE NEXT ENTRY WHEN THIS FUNCTION IS CALLED AGAIN
                        return entries_consumed;            // because we found an entry !!
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

                    out->lfn_entry_count++; // Update location !!

                }
            break;

            }
        }

        dir->current_cluster = get_next_cluster(filesystem, dir->current_cluster);
        dir->offset = 0;    // reset the offset
    }

    return false;   // EOF
}

bool try_make_name(fat32_info_t* filesystem, uint32_t first_cluster, const char* name, char* nameOut)
{
    char digit[] = "0123456789";

    char fatName[11];
    string_to_fatname(name, fatName);
    dir_iterator_t dir = {
        .current_cluster = first_cluster,
        .offset = 0,
    };
    fat_dir_entry_t entryOut;
    location_t out;
    char filename[257];
    int collision_count = 0;

    while(fat32_get_next_entry(filesystem, &dir, &entryOut, &out, filename) && collision_count < 10)
    {
        if(strncmp(fatName, entryOut.filename, 11) == 0)
        {
            collision_count++;
            int pos = strchr(name, '.') != NULL ? 6 : 9; // we need to preserve the extension if any

            // modifie the name in dos style
            fatName[pos++] = '~';
            fatName[pos] = digit[collision_count];

            // to restart the search !!
            dir.current_cluster = first_cluster;
            dir.offset = 0;
        }
    }

    if(collision_count >= 10)   return false;   // too munch collision

    memcpy(nameOut, fatName, 11);
    return true;
}

bool fat32_make_entry(fat32_info_t* filesystem, uint32_t first_cluster, const char* name, fat_attributes_t attribute)
{
    int name_length = strlen(name);
    if(name_length > 256)  return false;   // max 256 char for fat32

    char fatName[12] = {0};
    if(!try_make_name(filesystem, first_cluster, name, fatName)) return false;  // too munch collision
    
    uint8_t checksum = CalculateLFNChecksum(fatName);

    int lfn_entry_count = (name_length / 13) + ((name_length % 13) ? 1 : 0);    // basically rounding up
    uint32_t current_cluster = first_cluster;
    int free_entries = 0;

    int EntryCount = (filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector) / 32;

    dir_iterator_t startOf_lfn = {
        .current_cluster = current_cluster,
        .offset = 0,
    };


    for(;;)  // this loop is used to trace the cluster chain of the directory
    {

        readSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, current_cluster), filesystem->bootSector.sectors_per_cluster);

        fat_dir_entry_t* current_entry;

        for(int index = 0; index < EntryCount; index++) // this loop is used to parse the entries of a directory
        {
            current_entry = (fat_dir_entry_t*)filesystem->working_buffer + index;

            if((uint8_t)current_entry->filename[0] == 0xE5 || (uint8_t)current_entry->filename[0] == 0x00)  // free entry
            {
                if(!free_entries)   // if its the first one 
                {
                    startOf_lfn.current_cluster = current_cluster;
                    startOf_lfn.offset = index;
                }

                free_entries++;

                // if we got the required entries (plus 1 because of the 8.3 additional entry)
                if(free_entries == lfn_entry_count + 1) goto PutEntries;

                continue;
            }

            free_entries = 0;
        }

        uint32_t next_cluster = get_next_cluster(filesystem, current_cluster);
        if(!IS_VALID_CLUSTER(next_cluster))
        {
            next_cluster = alloc_newCluster(filesystem, current_cluster);
            if(!IS_VALID_CLUSTER(current_cluster))  return false;   // alloc new cluster failed

            // memeset the cluster
            readSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, next_cluster), filesystem->bootSector.sectors_per_cluster);
            memset(filesystem->working_buffer, 0, filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector);
            writeSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, next_cluster), filesystem->bootSector.sectors_per_cluster); // save change
        }

        current_cluster = next_cluster;
    }

PutEntries:
    int16_t name_16[13];
    char name_8[13];
    uint32_t processed_entries = 0;

    readSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, startOf_lfn.current_cluster), filesystem->bootSector.sectors_per_cluster);

    while(processed_entries < lfn_entry_count)
    {
        for(; startOf_lfn.offset < EntryCount && processed_entries < lfn_entry_count; startOf_lfn.offset++) // this loop is used to parse the entries of a directory
        {
            LFN* current_entry = (LFN*)filesystem->working_buffer + startOf_lfn.offset;

            memset(name_8, 0, 13);
            int name_index = (lfn_entry_count * 13) - (13 * (processed_entries + 1));
            int to_copy = name_index + 13 > name_length ? name_length - name_index : 13;   // checks overflow because
            strncpy(name_8, name + name_index, to_copy);        // we're copying the name from the back (stupid lfn)
            for(int i = 0; i < 13; i++) name_16[i] = name_8[i];

            current_entry->checksum = checksum;
            current_entry->attr = FAT_ATTR_LFN;
            current_entry->order = processed_entries ? lfn_entry_count - processed_entries : lfn_entry_count | 0x40;    // if its the first lfn entry sign it with 0x40
            current_entry->type = 0;
            current_entry->firstCluster = 0;
            
            memcpy(current_entry->name1, name_16, sizeof(uint16_t) * 5);
            memcpy(current_entry->name2, name_16 + 5, sizeof(uint16_t) * 6);
            memcpy(current_entry->name3, name_16 + 11, sizeof(uint16_t) * 2);

            processed_entries++;
        }

        if(processed_entries < lfn_entry_count || startOf_lfn.offset >= EntryCount) // if we havent processed all lfn or if we consumed all entries in the cluster
        {
            writeSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, startOf_lfn.current_cluster), filesystem->bootSector.sectors_per_cluster);  // flush modifications
            startOf_lfn.current_cluster = get_next_cluster(filesystem, startOf_lfn.current_cluster);
            startOf_lfn.offset = 0;
            readSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, startOf_lfn.current_cluster), filesystem->bootSector.sectors_per_cluster);   // load the new cluster
        }
    }

    fat_dir_entry_t* current_entry = (fat_dir_entry_t*)filesystem->working_buffer + startOf_lfn.offset;
    current_entry->attributes = attribute;
    memcpy(current_entry->filename, fatName, 11);
    current_entry->fileSize = 0;
    current_entry->firstClusterHigh = 0;
    current_entry->firstClusterLow = 0;
    current_entry->reserved = 0;
    // fat32_set_timestamps(current_entry);
    
    writeSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, startOf_lfn.current_cluster), filesystem->bootSector.sectors_per_cluster);  // flush modifications


    if(current_entry->attributes & FAT_ATTR_DIRECTORY)
    {
        uint32_t dir_first_cluster = alloc_newCluster(filesystem, 0);
        if(!IS_VALID_CLUSTER(dir_first_cluster)) return false; // dont really know how to handle this ...

        // memeset the cluster add the two generic posix compatible entries
        readSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, dir_first_cluster), filesystem->bootSector.sectors_per_cluster);
        memset(filesystem->working_buffer, 0, filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector);

        fat_dir_entry_t* entry = (fat_dir_entry_t*)filesystem->working_buffer;
        strncpy(entry->filename, ".          ", 11);   // this directory
        entry->attributes = FAT_ATTR_DIRECTORY;
        entry->firstClusterLow = dir_first_cluster;
        entry->firstClusterHigh = dir_first_cluster >> 16;

        entry = (fat_dir_entry_t*)(filesystem->working_buffer) + 1;
        strncpy(entry->filename, "..         ", 11);   // parent directory
        entry->attributes = FAT_ATTR_DIRECTORY;
        entry->firstClusterLow = first_cluster;
        entry->firstClusterHigh = first_cluster>> 16;

        writeSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, dir_first_cluster), filesystem->bootSector.sectors_per_cluster); // save change

        readSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, startOf_lfn.current_cluster), filesystem->bootSector.sectors_per_cluster);   // reload the cluster we were working on
        current_entry->firstClusterLow = dir_first_cluster;
        current_entry->firstClusterHigh = dir_first_cluster >> 16;
        writeSectors(filesystem->dev, filesystem->working_buffer, cluster_to_Lba(filesystem, startOf_lfn.current_cluster), filesystem->bootSector.sectors_per_cluster);  // re-flush modifications (yeah I hate it too)
    }

    return true;
}

bool fat32_lookup_in_dir(fat32_info_t* filesystem, uint32_t first_cluster, const char* name, fat_dir_entry_t* entryOut, location_t* out)
{
    dir_iterator_t dir = {
        .current_cluster = first_cluster,
        .offset = 0,
    };

    char filename[257];
    // char fatname[12];
    // string_to_fatname(name, fatname);

    while(fat32_get_next_entry(filesystem, &dir, entryOut, out, filename))
        if(!strCasecmp(name, filename)) return true;

    return false;
}

int fat32_readdir(fat32_info_t* filesystem, file_t* this_dir, dirEntry_t* out, uint64_t readPos)
{
    if((this_dir->entry.attributes & FAT_ATTR_DIRECTORY) != FAT_ATTR_DIRECTORY)
    {
        return -1;   // this isn't a directory
    }

    // in this context readPos represent AN OFFSET RELATIVE TO THE NUMBER OF FAT ENTRY !    

    uint32_t current_cluster = this_dir->entry.firstClusterLow | (this_dir->entry.firstClusterHigh << 16);
    uint32_t skipped_clusters = (uint32_t)(readPos * sizeof(fat_dir_entry_t) / (filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector));
    
    /* Some clusters are skipped since, based on the read position, their content doesn't need to be read. */
    for(int i = 0; i < skipped_clusters && IS_VALID_CLUSTER(current_cluster); i++)
        current_cluster = get_next_cluster(filesystem, current_cluster);

    int EntryCount = (filesystem->bootSector.sectors_per_cluster * filesystem->bootSector.bytes_per_sector) / 32;

    dir_iterator_t dir = {
        .current_cluster = current_cluster,
        .offset = readPos % EntryCount,
    };
    fat_dir_entry_t entry;
    location_t location;

    int entries_consumed = fat32_get_next_entry(filesystem, &dir, &entry, &location, out->name);
    if(entries_consumed)
    {
        out->type = (entry.attributes & FAT_ATTR_DIRECTORY) ? DIR_TYPE : FILE_TYPE;

        return readPos + entries_consumed;;
    }

    return 0; // EOF
}

bool fat32_make(fat32_info_t* filesystem, file_t* this_dir, char *name, bool isDirectory)
{
    if(strlen(name) > 256)  return false; // file name too large !!
    
    fat_dir_entry_t entryOut;
    location_t out;
    if(fat32_lookup_in_dir(filesystem, this_dir->entry.firstClusterLow | (this_dir->entry.firstClusterHigh << 16), name, &entryOut, &out))
        return false;   // file or directory already exist

    return fat32_make_entry(filesystem, this_dir->entry.firstClusterLow | (this_dir->entry.firstClusterHigh << 16), name, isDirectory ? FAT_ATTR_DIRECTORY : FAT_ATTR_REGULAR);
}