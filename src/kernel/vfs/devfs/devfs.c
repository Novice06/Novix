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
#include <string.h>
#include <mem_manager/heap.h>
#include <vfs/vfs.h>
#include <drivers/device.h>

#include "devfs.h"

#define MAX_VNODE 256

vnode_t* total_vnode[MAX_VNODE];

int devfs_getroot(vfs_t* mountpoint, vnode_t** result);
int devfs_mount(vfs_t* mountpoint, device_t* dev);
int devfs_unmount(vfs_t* mountpoint);

filesystem_t devfs_op = {
    // fs_name will be filled later
    .get_root = devfs_getroot,
    .VFS_mount = devfs_mount,
    .VFS_unmount = devfs_unmount,
};

static int64_t read(struct vnode* node, void *buffer, size_t size, int64_t offset, uint32_t flags);
static int64_t write(struct vnode* node, const void *buffer, size_t size, int64_t offset, uint32_t flags);
static int lookup(struct vnode* node_dir, const char* name, struct vnode** result);
static int ioctl(struct vnode* node, int command, void* arg);
int identify(struct vnode* node, uint64_t* fileSize);

// vnode operation !!
vnodeops_t devfs_vnode_op = {
    .read = read,
    .write = write,
    .lookup = lookup,
    .ioctl = ioctl,
    .identify = identify
};

void devfs_init()
{
    for(int i = 0; i < MAX_VNODE; i++)
        total_vnode[i] = NULL;

    strcpy(devfs_op.fs_name, "devfs");
    VFS_register_new_filesystem(&devfs_op);
}

int devfs_mount(vfs_t* mountpoint, device_t* dev)
{
    vnode_t* root = kmalloc(sizeof(vnode_t));

    if(!root)
        return VFS_ERROR;

    root->flags = VNODE_ROOT;
    root->VFS_mountedhere = NULL;
    root->vnode_data = NULL;
    root->vnode_op = &devfs_vnode_op;
    root->vnode_type = VDIR;
    root->vnode_vfs = mountpoint;

    mountpoint->vfs_data = root;

    return VFS_OK;
}

int devfs_unmount(vfs_t* mountpoint)
{
    kfree(mountpoint->vfs_data);

    return VFS_OK;
}

int devfs_getroot(vfs_t* mountpoint, vnode_t** result)
{
    *result = (vnode_t*)mountpoint->vfs_data;

    return VFS_OK;
}

static vnode_t* create_vnode(vfs_t* mountpoint, device_t* dev)
{
    for(int i = 0; i < MAX_VNODE; i++)
        if(total_vnode[i] != NULL && total_vnode[i]->vnode_data == (void*)dev)
            return total_vnode[i];    // if the vnode already exist in the vnode table

    vnode_t* newVnode = kmalloc(sizeof(vnode_t));
    newVnode->ref_count = 0;
    newVnode->flags = VNODE_NONE;
    newVnode->VFS_mountedhere = NULL;
    newVnode->vnode_data = dev;    // devfs store the device here !!
    newVnode->vnode_op = &devfs_vnode_op;
    newVnode->vnode_vfs = mountpoint;
    newVnode->vnode_type = VREG;
   
    // searching for a free place
    for(int i = 0; i < MAX_VNODE; i++)
    {   
        if(total_vnode[i] == NULL)
        {
            total_vnode[i] = newVnode;
            return newVnode;
        } 

        // if the vnode is unused
        if(total_vnode[i]->ref_count <= 0)
        {
            kfree(total_vnode[i]);
            total_vnode[i] = newVnode;
            return newVnode;
        }
    }

    kfree(newVnode);
    return NULL;    // cannot create vnode
}

int64_t read(struct vnode* node, void *buffer, size_t size, int64_t offset, uint32_t flags)
{
    device_t* dev = (device_t*)node->vnode_data;

    return dev->read(buffer, offset, size, dev->priv, flags);
}

int64_t write(struct vnode* node, const void *buffer, size_t size, int64_t offset, uint32_t flags)
{
    device_t* dev = (device_t*)node->vnode_data;

    return dev->write(buffer, offset, size, dev->priv, flags);
}

int lookup(struct vnode* node_dir, const char* name, struct vnode** result)
{
    vnode_t* root = (vnode_t*)node_dir->vnode_vfs->vfs_data;

    if (node_dir != root)
        return VFS_ENOTDIR;

    device_t* dev = lookup_device(name);
    if(!dev)
        return VFS_ENOENT;

    vnode_t* res = create_vnode(node_dir->vnode_vfs, dev);
    if(!res)
        return VFS_ENFILE;

    *result = res;
    return VFS_OK;
}

int ioctl(struct vnode* node, int command, void* arg)
{
    device_t* dev = (device_t*)node->vnode_data;
    return dev->ioctl(command, arg);
}

int identify(struct vnode* node, uint64_t* fileSize)
{
    if(fileSize)
        *fileSize = 0;

    return VFS_OK;
}