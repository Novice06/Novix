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

#include <debug.h>
#include <stdbool.h>
#include <string.h>
#include <memory.h>
#include <mem_manager/heap.h>
#include <drivers/device.h>
#include <vfs/vfs.h>

#include "ramfs.h"

// static uint64_t get_current_time() {
//     return (uint64_t)time(NULL);
// }

static treenode_t* ramfs_lookup(treenode_t *dir, const char *name)
{
    if (!dir || dir->meta.type != NODE_DIRECTORY)
        return NULL;
    
    // // update
    // dir->meta.access_time = get_current_time();
    
    treenode_t *child = dir->first_child;
    while (child)
    {
        if (strcmp(child->meta.name, name) == 0)
            return child;
        
        child = child->next_sibling;
    }
    
    return NULL;
}

static treenode_t* ramfs_create_node(treenode_t *parent, const char *name, nodetype_t type)
{
    if (!parent || parent->meta.type != NODE_DIRECTORY)
        return NULL;
    
    // verifies if the node doesn't already exists
    if (ramfs_lookup(parent, name) != NULL)
        return NULL; // Le nom existe déjà
    
    treenode_t *new_node = (treenode_t*)kmalloc(sizeof(treenode_t));
    if (!new_node)
        return NULL;
    
    // initialize metadata
    strncpy(new_node->meta.name, name, VFS_MAX_FILENAME - 1);
    new_node->meta.name[VFS_MAX_FILENAME - 1] = '\0';
    new_node->meta.type = type;
    new_node->meta.size = 0;
    // uint64_t current_time = get_current_time();
    // new_node->meta.create_time = current_time;
    // new_node->meta.modify_time = current_time;
    // new_node->meta.access_time = current_time;
    
    // initialize links
    new_node->parent = parent;
    new_node->first_child = NULL;
    new_node->next_sibling = NULL;
    new_node->data = NULL;
    
    // add to the parent's child list
    if (!parent->first_child)
        parent->first_child = new_node;
    else
    {
        treenode_t *sibling = parent->first_child;
        while (sibling->next_sibling)
            sibling = sibling->next_sibling;

        sibling->next_sibling = new_node;
    }
    
    // update !
    // parent->meta.modify_time = current_time;
    
    return new_node;
}

static int64_t ramfs_write(treenode_t *file, const uint8_t *data, uint64_t size, uint64_t offset)
{
    if (!file || file->meta.type != NODE_FILE)
        return VFS_ENOENT;
    
    uint64_t new_size = (offset + size > file->meta.size) ? offset + size : file->meta.size;
    
    // allocate memory if necessary
    if (new_size > file->meta.size || !file->data) 
    {
        uint8_t *new_data = (uint8_t*)krealloc(file->data, new_size);
        if (!new_data)
            return VFS_ERROR;
        
        file->data = new_data;
    }
    
    if (data && size > 0)
        memcpy(file->data + offset, data, size);
    
    // update file size
    file->meta.size = new_size;
    
    // uint64_t current_time = get_current_time();
    // file->meta.modify_time = current_time;
    // file->meta.access_time = current_time;
    
    return size;
}

static int64_t ramfs_read(treenode_t *file, uint8_t *buffer, uint64_t size, uint64_t offset)
{
    if (!file || file->meta.type != NODE_FILE || !buffer)
        return VFS_ENOENT;
    
    // file->meta.access_time = get_current_time();
    
    // EOF ?
    if (offset >= file->meta.size)
        return 0;
    
    // calculate byte to read
    uint64_t to_read = (offset + size > file->meta.size) ? file->meta.size - offset : size;
    
    memcpy(buffer, file->data + offset, to_read);

    return to_read;
}

#define MAX_VNODE_PER_VFS   256

/* Important information for the file system ! */
typedef struct ramfs_info
{
    vnode_t* total_vnode[MAX_VNODE_PER_VFS];
    vnode_t* root_vnode;
    treenode_t* root_node;
}fs_info_t;

int ramfs_mount(vfs_t* mountpoint, device_t* dev);
int ramfs_unmount(vfs_t* mountpoint);
int ramfs_get_root(vfs_t* mountpoint, vnode_t** result);

static int64_t read(vnode_t* node, void *buffer, size_t size, int64_t offset, uint32_t flags);
static int64_t write(vnode_t* node, const void *buffer, size_t size, int64_t offset, uint32_t flags);
static int lookup(vnode_t* node, const char* name, struct vnode** result);

filesystem_t ramfs_op = {
    // fs_name will be filled later
    .get_root = ramfs_get_root,
    .VFS_mount = ramfs_mount,
    .VFS_unmount = ramfs_unmount,
};

// vnode operation !!
vnodeops_t ramfs_vnode_op = {
    .read = read,
    .write = write,
    .lookup = lookup,
    .ioctl = NULL,
};

void ramfs_init()
{
    strcpy(ramfs_op.fs_name, "ramfs");
    VFS_register_new_filesystem(&ramfs_op);
}

treenode_t* ramfs_createRoot()
{
    treenode_t* root_fs = kmalloc(sizeof(treenode_t));
    root_fs->meta.type = NODE_DIRECTORY;
    root_fs->parent = NULL;
    root_fs->first_child = NULL;
    root_fs->next_sibling = NULL;

    treenode_t* test = ramfs_create_node(root_fs, "test", NODE_DIRECTORY);
    treenode_t* hello = ramfs_create_node(test, "hello.txt", NODE_FILE);

    ramfs_write(hello, (const uint8_t*)"hello world !", 13, 0);

    ramfs_create_node(root_fs, "dev", NODE_DIRECTORY);
    ramfs_create_node(root_fs, "mnt", NODE_DIRECTORY);

    return root_fs;
}

/*
 * Mounts a RAM-based file system on the given mount point.
 *
 * Mounting a ramfs device simply involves retrieving the memory address
 * where the file system resides. This information is stored in the 'priv'
 * field of the corresponding device structure.
 *
 * The mountpoint private data is then linked to the filesystem specific info,
 * enabling the VFS to access its contents.
 */
int ramfs_mount(vfs_t* mountpoint, device_t* dev)
{
    fs_info_t* fs_info = kmalloc(sizeof(fs_info_t));

    for(int i = 0; i < MAX_VNODE_PER_VFS; i++)
        fs_info->total_vnode[i] = NULL;

    fs_info->root_node = ramfs_createRoot();

    fs_info->root_vnode = kmalloc(sizeof(vnode_t));
    fs_info->root_vnode->ref_count = 0;
    fs_info->root_vnode->flags = VNODE_NONE;
    fs_info->root_vnode->vnode_type = VDIR;
    fs_info->root_vnode->VFS_mountedhere = NULL;
    fs_info->root_vnode->vnode_op = &ramfs_vnode_op;
    fs_info->root_vnode->vnode_vfs = mountpoint;
    fs_info->root_vnode->vnode_data = fs_info->root_node;

    // here we need to fill specific filesystem info !
    mountpoint->vfs_data = fs_info;

    return VFS_OK;
}

int ramfs_unmount(vfs_t* mountpoint)
{
    fs_info_t* fs_info = (fs_info_t*)mountpoint->vfs_data;

    for(int i = 0; i < MAX_VNODE_PER_VFS; i++)
        if(fs_info->total_vnode[i] != NULL)
            kfree(fs_info->total_vnode[i]);

    kfree(fs_info->root_vnode);

    kfree(fs_info);

    // we need to deallocate the tree datastructure of this filesystem ...

    return VFS_OK;
}

int ramfs_get_root(vfs_t* mountpoint, vnode_t** result)
{
    fs_info_t* fs_info = (fs_info_t*)mountpoint->vfs_data;

    *result = fs_info->root_vnode;
    
    return VFS_OK;
}

/*
 * Creates a vnode for the given tree node within the specified mount point.
 *
 * This function checks the vnode table of the file system to see if a vnode
 * already exists for the given tree node. If it does, that vnode is returned.
 * Otherwise, a new vnode is created and added to the table.
 *
 * If there is no space left in the vnode table for a new entry, the function
 * returns NULL.
 */
static vnode_t* create_vnode(vfs_t* mountpoint, treenode_t* node)
{
    fs_info_t* fs_info = (fs_info_t*)mountpoint->vfs_data;

    for(int i = 0; i < MAX_VNODE_PER_VFS; i++)
        if(fs_info->total_vnode[i] != NULL && fs_info->total_vnode[i]->vnode_data == (void*)node)
            return fs_info->total_vnode[i];    // if the vnode already exist in the vnode table

    vnode_t* newVnode = kmalloc(sizeof(vnode_t));
    newVnode->ref_count = 0;
    newVnode->flags = VNODE_NONE;
    newVnode->VFS_mountedhere = NULL;
    newVnode->vnode_data = node;    // ramfs store the node here !!
    newVnode->vnode_op = &ramfs_vnode_op;
    newVnode->vnode_vfs = mountpoint;

    switch (node->meta.type)
    {
    case NODE_DIRECTORY:
        newVnode->vnode_type = VDIR;
        break;
    
    case NODE_FILE:
        newVnode->vnode_type = VREG;    
        break;

    default:
        newVnode->vnode_type = VNON;
        break;
    }
   
    // searching for a free place
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
            kfree(fs_info->total_vnode[i]);
            fs_info->total_vnode[i] = newVnode;
            return newVnode;
        }
    }

    kfree(newVnode);
    return NULL;    // cannot create vnode
}

int lookup(vnode_t* node_dir, const char* name, struct vnode** result)
{
    treenode_t* node = NULL;
    treenode_t* parent = (treenode_t*)node_dir->vnode_data;

    node = ramfs_lookup(parent, name);
    if(node == NULL)
        return VFS_ENOENT;

    *result = create_vnode(node_dir->vnode_vfs, node);

    if(*result == NULL)
        return VFS_ERROR;
    else
        return VFS_OK;
}

int64_t read(vnode_t* node, void *buffer, size_t size, int64_t offset, uint32_t flags)
{
    treenode_t* file_node = (treenode_t*)node->vnode_data;

    return ramfs_read(file_node, buffer, size, offset);
}

int64_t write(vnode_t* node, const void *buffer, size_t size, int64_t offset, uint32_t flags)
{
    treenode_t* file_node = (treenode_t*)node->vnode_data;

    return ramfs_write(file_node, buffer, size, offset);
}