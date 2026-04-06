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

#include <drivers/device.h>
#include <mem_manager/heap.h>
#include <memory.h>
#include <string.h>
#include <vfs/vfs.h>
#include <debug.h>

#include "fat32_interface.h"
#include "fat32.h"

int fat32_mount(vfs_t* mountpoint, device_t* dev);
int fat32_unmount(vfs_t* mountpoint);
int fat32_get_root(vfs_t* mountpoint, vnode_t** result);

static int64_t read(vnode_t* node, void *buffer, size_t size, int64_t offset, uint32_t flags);
static int64_t write(vnode_t* node, const void *buffer, size_t size, int64_t offset, uint32_t flags);
static int ioctl(struct vnode* node, const int command, void* arg);
static int lookup(vnode_t* node, const char* name, struct vnode** result);
static int trunc(struct vnode* node);
static int create(struct vnode* node_dir, const char* name, vtype type);
static int remove(struct vnode* node);
static int readdir(struct vnode* node_dir, struct dirent* buffer, uint32_t count);
static int stat(struct vnode* node, vfs_stat_t* stat);

filesystem_t fat32_op = {
    // fs_name will be filled later
    .get_root = fat32_get_root,
    .VFS_mount = fat32_mount,
    .VFS_unmount = fat32_unmount,
};

// vnode operation !!
vnodeops_t fat32_vnode_op = {
    .read = read,
    .write = write,
    .lookup = lookup,
    .ioctl = ioctl,
    .trunc = trunc,
    .create = create,
    .remove = remove,
    .readdir = readdir,
    .stat = stat,
};

void fat12_init()
{
    strcpy(fat32_op.fs_name, "fat32");
    VFS_register_new_filesystem(&fat32_op);
}

int fat32_mount(vfs_t* mountpoint, device_t* dev)
{
    if(!dev) return VFS_ERROR;

    fat32_info_t* fs_info = kmalloc(sizeof(fat32_info_t));

    if(fs_info == NULL)
        return VFS_ERROR;

    fs_info->dev = dev;

    for(int i = 0; i < MAX_VNODE_PER_VFS; i++)
        fs_info->total_vnode[i] = NULL;

    dev->read((void*)&fs_info->bootSector, 0, 1, dev->priv, 0);

    fs_info->FAT = kmalloc(fs_info->bootSector.bytes_per_sector);    // we will be reading the fat by sector chunk
    if(fs_info->FAT == NULL)
    {
        kfree(fs_info);
        return VFS_ERROR;
    }

    fs_info->working_buffer = kmalloc(fs_info->bootSector.sectors_per_cluster * fs_info->bootSector.bytes_per_sector);
    if(fs_info->working_buffer == NULL)
    {
        kfree(fs_info->FAT);
        kfree(fs_info);
        return VFS_ERROR;
    }

    fs_info->root_vnode = kmalloc(sizeof(vnode_t));
    if(fs_info->root_vnode == NULL)
    {
        kfree(fs_info->FAT);
        kfree(fs_info->working_buffer);
        kfree(fs_info);
        return VFS_ERROR;
    }

    file_t* root = kmalloc(sizeof(file_t));
    memset(root, 0, sizeof(file_t));
    root->entry.attributes = FAT_ATTR_DIRECTORY;
    root->entry.firstClusterLow = fs_info->bootSector.root_cluster;
    root->entry.firstClusterHigh = fs_info->bootSector.root_cluster >> 16;
    root->isRoot = true;

    fs_info->root_vnode->ref_count = 0;
    fs_info->root_vnode->flags = VNODE_ROOT;
    fs_info->root_vnode->vnode_type = VDIR;
    fs_info->root_vnode->VFS_mountedhere = NULL;
    fs_info->root_vnode->vnode_op = &fat32_vnode_op;
    fs_info->root_vnode->vnode_vfs = mountpoint;
    fs_info->root_vnode->vnode_data = root; // its the root !!

    // here we need to fill specific filesystem info !
    mountpoint->vfs_data = fs_info;

    return VFS_OK;
}

int fat32_unmount(vfs_t* mountpoint)
{
    fat32_info_t* fs_info = (fat32_info_t*)mountpoint->vfs_data;

    for(int i = 0; i < MAX_VNODE_PER_VFS; i++)
    {
        if(fs_info->total_vnode[i] != NULL)
        {
            kfree(fs_info->total_vnode[i]->vnode_data);
            kfree(fs_info->total_vnode[i]);
        }
    }
    
    kfree(fs_info->root_vnode);
    kfree(fs_info->FAT);
    kfree(fs_info->working_buffer);
    kfree(fs_info);

    return VFS_OK;
}

int fat32_get_root(vfs_t* mountpoint, vnode_t** result)
{
    fat32_info_t* fs_info = (fat32_info_t*)mountpoint->vfs_data;

    *result = fs_info->root_vnode;
    
    return VFS_OK;
}

int ioctl(struct vnode* node, const int command, void* arg)
{
    return VFS_OK;
}

static vnode_t* create_vnode(vfs_t* mountpoint, file_t* inode_info)
{
    fat32_info_t* fs_info = (fat32_info_t*)mountpoint->vfs_data;

    /* Here we try to determine if the vnode of the target element is already present in the cache. */
    for(int i = 0; i < MAX_VNODE_PER_VFS; i++)
    {
        if(fs_info->total_vnode[i] != NULL)
        {
            file_t* existing_inode = (file_t*)fs_info->total_vnode[i]->vnode_data;

            uint32_t existing_firstCluster = existing_inode->entry.firstClusterLow | (existing_inode->entry.firstClusterHigh << 16);
            uint32_t new_firstCluster = inode_info->entry.firstClusterLow | (inode_info->entry.firstClusterHigh << 16);

            if(strncmp(existing_inode->entry.filename, inode_info->entry.filename, 11) == 0 && existing_firstCluster == new_firstCluster)
                return fs_info->total_vnode[i];    // if the vnode already exist in the vnode table
        }
    }

    /* Otherwise, we create a new vnode and ensure that we also generate a new inode,
    since the one we received is temporary (as it came from the FAT buffer). */
    file_t* file_inode = kmalloc(sizeof(file_t));
    memcpy(file_inode, inode_info, sizeof(file_t));

    vnode_t* newVnode = kmalloc(sizeof(vnode_t));
    newVnode->flags = VNODE_NONE;
    newVnode->ref_count = 0;
    newVnode->VFS_mountedhere = NULL;
    newVnode->vnode_data = file_inode;    // we store the inode here !!
    newVnode->vnode_op = &fat32_vnode_op;
    newVnode->vnode_vfs = mountpoint;

    if((file_inode->entry.attributes & FAT_ATTR_DIRECTORY) == FAT_ATTR_DIRECTORY)
        newVnode->vnode_type = VDIR;
    else
        newVnode->vnode_type = VREG;

    // we'll search a free place in the cache
    for(int i = 0; i < MAX_VNODE_PER_VFS; i++)
    {   
        if(fs_info->total_vnode[i] == NULL)
        {
            fs_info->total_vnode[i] = newVnode;
            return newVnode;
        } 

        // if the vnode is unused
        if(fs_info->total_vnode[i]->ref_count <= 0)
        {
            kfree(fs_info->total_vnode[i]->vnode_data);  // free the inode !
            kfree(fs_info->total_vnode[i]);

            fs_info->total_vnode[i] = newVnode;
            return newVnode;
        }
    }

    kfree(newVnode->vnode_data); // free the inode !
    kfree(newVnode);
    return NULL;    // cannot create vnode because too many vnodes are in used
}

int64_t read(vnode_t* node, void *buffer, size_t size, int64_t offset, uint32_t flags)
{
    file_t* inode = node->vnode_data;
    fat32_info_t* fs_info = node->vnode_vfs->vfs_data;

    int64_t read = fat32_read(fs_info, inode, buffer, size, offset);
    return read < 0 ? VFS_ERROR : read;
}

int64_t write(vnode_t* node, const void *buffer, size_t size, int64_t offset, uint32_t flags)
{
    file_t* inode = node->vnode_data;
    fat32_info_t* fs_info = node->vnode_vfs->vfs_data;

    int64_t read = fat32_write(fs_info, inode, buffer, size, offset);
    return read < 0 ? VFS_ERROR : read;
}

int lookup(vnode_t* node, const char* name, struct vnode** result)
{
    if(node->vnode_type != VDIR)
    {
        *result = NULL;
        return VFS_ENOTDIR;
    }
    
    file_t* inode = node->vnode_data;
    fat32_info_t* fs_info = node->vnode_vfs->vfs_data;

    file_t* new = kmalloc(sizeof(file_t));

    if(!fat32_lookup_in_dir(fs_info, inode->entry.firstClusterLow | (inode->entry.firstClusterHigh << 16), name, &new->entry, &new->in_parent))
    {
        kfree(new);
        *result = NULL;
        return VFS_ENOENT;
    }

    new->isRoot = false;

    *result = create_vnode(node->vnode_vfs, new);
    if(*result == NULL)
    {
        kfree(new);
        return VFS_ENFILE;
    }

    // if we already had this file cached then the allocation was useless
    if((*result)->vnode_data != new)    kfree(new); // we must free this inode otherwise its a memory leak
    return VFS_OK;
}

int create(struct vnode* node_dir, const char* name, vtype type)
{
    file_t* dir = node_dir->vnode_data;
    fat32_info_t* fs_info = node_dir->vnode_vfs->vfs_data;

    if(fat32_make(fs_info, dir, name, type == VDIR ? true : false)) return VFS_OK;

    return VFS_ERROR;
}

int trunc(struct vnode* node)
{
    file_t* file = node->vnode_data;
    fat32_info_t* fs_info = node->vnode_vfs->vfs_data;

    fat32_trunc(fs_info, file);
    return VFS_OK;
}

int remove(struct vnode* node)
{
    file_t* file = node->vnode_data;
    fat32_info_t* fs_info = node->vnode_vfs->vfs_data;

    fat32_delete(fs_info, file);
    return VFS_OK;
}


int stat(struct vnode* node, vfs_stat_t* stat)
{
    file_t* file = node->vnode_data;

    stat->accessed = file->entry.lastAccessDate;
    stat->created  = file->entry.creationTime;
    stat->modified = file->entry.creationTime;
    stat->permissions = 0;
    stat->size = file->entry.fileSize;
    stat->type = node->vnode_type;

    return VFS_OK;
}

int readdir(struct vnode* node_dir, struct dirent* buffer, uint32_t count)
{
    file_t* dir = node_dir->vnode_data;
    fat32_info_t* fs_info = node_dir->vnode_vfs->vfs_data;

    dirEntry_t entry;
    int readpos = 0;
    int entries = 0;

    do
    {
        readpos = fat32_readdir(fs_info, dir, &entry, readpos);;

        if(readpos)
        {
            if(strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) continue;   // these are unix compatible directories (doesnt count as entries)

            strncpy(buffer[entries].name, entry.name, 256);
            buffer[entries].type = entry.type;
            entries++;
        }
    } while (readpos && entries < count);
    

    return entries;
}