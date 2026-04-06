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
#include <drivers/vga_text.h>
#include <drivers/e9_port.h>
#include <drivers/device.h>
#include <mem_manager/heap.h>
#include <string.h>
#include <vfs/vfs.h>
#include <multitasking/process.h>
#include <multitasking/scheduler.h>

#include "ramfs/ramfs.h"
#include "devfs/devfs.h"
#include "fat32/fat32.h"

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define VFS_MAX_FS 10

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

vfs_t *vfs_root;
filesystem_t *registered_fs[VFS_MAX_FS];
int num_registered_fs;

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

static void add_mount_point(vfs_t *mountpoint)
{
	if(vfs_root == NULL)
	{
		vfs_root = mountpoint;
		mountpoint->next = NULL;
		return;
	}

	vfs_t *current = vfs_root;
	while (current->next != NULL)
		current = current->next;

	current->next = mountpoint;
	mountpoint->next = NULL;
}

static void remove_mount_point(vfs_t *mountpoint)
{
	vfs_t *current = vfs_root;

	while (current->next != mountpoint)
		current = current->next;

	current->next = mountpoint->next;
}

 static filesystem_t *find_filesystem_by_name(const char *name)
 {
	for (int i = 0; i < num_registered_fs; i++)
	{
		if (strcmp(registered_fs[i]->fs_name, name) == 0)
		{
			return registered_fs[i];
		}
	}
	return NULL;
 }

static int find_free_fd(void)
{
	for (int i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (PROCESS_getCurrent()->resources[i].vnode == NULL)
			return i;
	}
	return VFS_ENFILE;
}

static int is_fd_valid(int fd)
{
	if(fd < 0 || fd >= MAX_OPEN_FILES)
		return 0;

	// if the fd is currently in use ...
	if (PROCESS_getCurrent()->resources[fd].vnode == NULL)
		return 0;

	return 1;
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void VFS_init()
{
	vfs_root = NULL;		// 0 mountpoint
	num_registered_fs = 0;

	for(int i = 0; i < VFS_MAX_FS; i++)
		registered_fs[i] = NULL;

	ramfs_init();
	devfs_init();
	fat12_init();
}

vnode_t* lookup_path_name(const char* path)
{
	vnode_t* node_out = NULL;
	char parsed_path[VFS_MAX_PATH_LENGTH];
	char* name;
    char* next_name;

	if(path == NULL || path[0] != '/')
		return NULL;

	strcpy(parsed_path, path);

	vfs_root->vfs_op->get_root(vfs_root, &node_out);
	name = parsed_path + 1;   // ignore the first '/'
    next_name = name;

	while(node_out != NULL && *name != '\0')
	{
		if(node_out->VFS_mountedhere != NULL) // if this is a mountpoint
			node_out->VFS_mountedhere->vfs_op->get_root(node_out->VFS_mountedhere, &node_out);

        next_name = (char*)strchr(next_name, '/');
        if(next_name != NULL)
        {
            *next_name = '\0';
            next_name++;
        }
        else
        {
            while (*next_name != '\0')
                next_name++;
        }

		node_out->vnode_op->lookup(node_out, name, &node_out);
		name = next_name;
	}

	// If `name` is not NULL, it means the path has not been fully parsed yet.
	// This check is commented out because it's unnecessary:
	// the `lookup` vnode operation will already return NULL if the file is not found.
	//if(name != NULL)
	//	return NULL;

	if(node_out != NULL && node_out->VFS_mountedhere != NULL) // if this is a mountpoint
			node_out->VFS_mountedhere->vfs_op->get_root(node_out->VFS_mountedhere, &node_out);
	
	return node_out;
}

int VFS_mount(const char *fs_name, device_t* dev, const char *mount_point)
{
	vfs_t *new_vfs;
	filesystem_t *fs;

	fs = find_filesystem_by_name(fs_name);

	if(fs == NULL)
		return VFS_ERROR; // error code !

	new_vfs = kmalloc(sizeof(vfs_t));

	new_vfs->next = NULL;
	new_vfs->vfs_op = fs;

	if(vfs_root == NULL)	// is this the first mount point ?
		new_vfs->vnodecovered = NULL;
	else
	{
		// find the vnode's mountpoint
		new_vfs->vnodecovered = lookup_path_name(mount_point);
		if(new_vfs->vnodecovered == NULL || (new_vfs->vnodecovered->flags & VNODE_ROOT) == VNODE_ROOT)
		{
			kfree(new_vfs);
			return VFS_ENOENT;
		}

		if(new_vfs->vnodecovered->vnode_type != VDIR)
		{
			kfree(new_vfs);
			return VFS_ENOTDIR;
		}

		new_vfs->vnodecovered->ref_count++;
		new_vfs->vnodecovered->VFS_mountedhere = new_vfs;
	}

	if(new_vfs->vfs_op->VFS_mount(new_vfs, dev) != VFS_OK)
        return VFS_ERROR;
    
	add_mount_point(new_vfs);

	return VFS_OK;	// ok
}

 int VFS_unmount(const char *mount_point)
 {
	vnode_t* vnode = lookup_path_name(mount_point);
	if(vnode == NULL)
		return VFS_ENOENT;

	if((vnode->flags & VNODE_ROOT) != VNODE_ROOT)
		return VFS_ERROR; // it's not the root of a filesystem, it's not a mount point...

	vfs_t* mountpoint = vnode->vnode_vfs;

	if (mountpoint == vfs_root)
         return VFS_EACCESS;  // cannot unmount the root fs

	// TODO: implemente a mechanism to prevent umounting a filesystem
	// as long as there are other filesystems mounted on top of it

	// this vnode is no longer a mountpoint
	mountpoint->vnodecovered->VFS_mountedhere = NULL;
	mountpoint->vnodecovered->ref_count--;

	mountpoint->vfs_op->VFS_unmount(mountpoint);
	remove_mount_point(mountpoint);
	kfree(mountpoint);

    return VFS_OK;
 }

 int VFS_open(const char *path, uint16_t mode)
 {
	vnode_t* file_node = lookup_path_name(path);

	if(file_node == NULL)
	{
		if((mode & VFS_O_CREAT) != VFS_O_CREAT) return VFS_ENOENT;

		char name[257];
		char* parsed_path = kmalloc(sizeof(char) * (strlen(path) + 1));
		strcpy(parsed_path, path);

		for(int i = strlen(path) - 1; i >= 0; i--)
		{
			if(parsed_path[i] == '/')
			{
				if(strlen(parsed_path + i + 1) > 256)
				{
					kfree(parsed_path);
					return VFS_ERROR; // file name too large !!
				}

				strcpy(name, parsed_path + i + 1);
				parsed_path[i+1] = '\0';
				break;
			}
		}

		vnode_t* parent = lookup_path_name(parsed_path);
		if(!parent)
		{
			kfree(parsed_path);
			return VFS_ENOENT;
		}

		parent->vnode_op->create(parent, name, VREG);
		kfree(parsed_path);

		file_node = lookup_path_name(path);	// retry
		if(!file_node)	return VFS_ERROR;
	}

	if(file_node->vnode_type != VREG)
		return VFS_EISDIR;

	int descriptor = find_free_fd();
	if(descriptor == VFS_ENFILE)
		return VFS_ENFILE;

	file_node->ref_count++;

	PROCESS_getCurrent()->resources[descriptor].mode = mode;
	PROCESS_getCurrent()->resources[descriptor].position = 0;
	PROCESS_getCurrent()->resources[descriptor].vnode = file_node;

	if((mode & VFS_O_TRUNC) == VFS_O_TRUNC) file_node->vnode_op->trunc(file_node);
	if((mode & VFS_O_APPEND) == VFS_O_APPEND)	VFS_seek(descriptor, 0, VFS_SEEK_END);

	return descriptor;
 }

int VFS_close(int fd)
{
	if(!is_fd_valid(fd))
		return VFS_EBADF;

	PROCESS_getCurrent()->resources[fd].vnode->ref_count--;
	PROCESS_getCurrent()->resources[fd].vnode = NULL;

    return VFS_OK;
}

int64_t VFS_read(int fd, void *buffer, size_t size)
{
	if(!is_fd_valid(fd))
		return VFS_EBADF;

	if(!(PROCESS_getCurrent()->resources[fd].mode & VFS_O_RDONLY) && !(PROCESS_getCurrent()->resources[fd].mode & VFS_O_RDWR))
		return VFS_EACCESS;

	int ret = PROCESS_getCurrent()->resources[fd].vnode->vnode_op->read(PROCESS_getCurrent()->resources[fd].vnode, buffer, size, PROCESS_getCurrent()->resources[fd].position, PROCESS_getCurrent()->resources[fd].mode);

	if(ret < 0)	// it's an error
		return ret;

	PROCESS_getCurrent()->resources[fd].position += ret;

	return ret;
}

int64_t VFS_write(int fd, const void *buffer, size_t size)
{
	switch (fd)
    {
    case VFS_FD_STDOUT:
    case VFS_FD_STDERR:
        // for (size_t i = 0; i < size; i++)
        //     VGA_putc(*((uint8_t*)buffer+i));
        // return size;

    case VFS_FD_DEBUG:
        for (size_t i = 0; i < size; i++)
			E9_putc((*(uint8_t*)buffer+i));
        return size;
    }

	// else here:

	if(!is_fd_valid(fd))
		return VFS_EBADF;

	if(!(PROCESS_getCurrent()->resources[fd].mode & VFS_O_WRONLY) && !(PROCESS_getCurrent()->resources[fd].mode & VFS_O_RDWR))
		return VFS_EACCESS;

	int ret = PROCESS_getCurrent()->resources[fd].vnode->vnode_op->write(PROCESS_getCurrent()->resources[fd].vnode, buffer, size, PROCESS_getCurrent()->resources[fd].position, PROCESS_getCurrent()->resources[fd].mode);
	
	if(ret < 0)	// it's an error
		return ret;

	PROCESS_getCurrent()->resources[fd].position += ret;

	return ret;
}

int VFS_ioctl(int fd, int command, void* arg)
{
	if(!is_fd_valid(fd))
		return VFS_EBADF;

	return PROCESS_getCurrent()->resources[fd].vnode->vnode_op->ioctl(PROCESS_getCurrent()->resources[fd].vnode, command, arg);
}

size_t VFS_seek(int fd, int64_t offset, int whence)
{
	if(!is_fd_valid(fd))
		return VFS_EBADF;

	switch (whence)
	{
	case VFS_SEEK_SET:
		PROCESS_getCurrent()->resources[fd].position = offset;
		break;
	
	case VFS_SEEK_CUR:
		PROCESS_getCurrent()->resources[fd].position += offset;
		break;

	case VFS_SEEK_END:
		uint64_t file_size;
		vfs_stat_t info;
		PROCESS_getCurrent()->resources[fd].vnode->vnode_op->stat(PROCESS_getCurrent()->resources[fd].vnode, &info);
		PROCESS_getCurrent()->resources[fd].position = info.size + offset;
		break;
	
	default:
		break;
	}

	return PROCESS_getCurrent()->resources[fd].position;
}

int VFS_mkdir(const char* path, uint16_t mode)
{
	if(lookup_path_name(path)) return VFS_EEXIST;	// already exists

	char name[257];
    char* parsed_path = kmalloc(sizeof(char) * (strlen(path) + 1));
    strcpy(parsed_path, path);

    for(int i = strlen(path) - 1; i >= 0; i--)
    {
        if(parsed_path[i] == '/')
        {
            if(strlen(parsed_path + i + 1) > 256)
			{
				kfree(parsed_path);
				return VFS_ERROR; // file name too large !!
			}

            strcpy(name, parsed_path + i + 1);
            parsed_path[i+1] = '\0';
            break;
        }
    }

	vnode_t* parent = lookup_path_name(parsed_path);
	if(!parent)
	{
		kfree(parsed_path);
		return VFS_ENOENT;
	}

	int ret = parent->vnode_op->create(parent, name, VDIR);
	kfree(parsed_path);

	return ret;
}

int VFS_rmdir(const char* path)
{
	vnode_t* dir = lookup_path_name(path);
	if(!dir)	return VFS_ENOENT;
	if(dir->vnode_type != VDIR) return VFS_ENOTDIR;

	struct dirent buff = {
		.name = {0},
		.type = 0,
	};
	dir->vnode_op->readdir(dir, &buff, 1);
	if(buff.name[0])	return VFS_ERROR;	// directory not empty

	if(dir->ref_count)	return VFS_EAGAIN;	// in use !!

	return dir->vnode_op->remove(dir);
}

int VFS_unlink(const char* path)
{
	vnode_t* file = lookup_path_name(path);
	if(!file)	return VFS_ENOENT;
	if(file->vnode_type == VDIR) return VFS_EISDIR;

	if(file->ref_count) return VFS_EAGAIN;

	return file->vnode_op->remove(file);
}

int VFS_stat(const char* path, vfs_stat_t* stat)
{
	vnode_t* node = lookup_path_name(path);
	if(!node) return VFS_ENOENT;

	return node->vnode_op->stat(node, stat);
}

int VFS_getdents(const char* path, struct dirent* entries, uint32_t count)
{
	vnode_t* dir = lookup_path_name(path);
	if(!dir)	return VFS_ENOENT;
	if(dir->vnode_type != VDIR) return VFS_ENOTDIR;

	return dir->vnode_op->readdir(dir, entries, count);
}

void VFS_register_new_filesystem(filesystem_t* fs)
{
	if(num_registered_fs >= VFS_MAX_FS)
		return;

	registered_fs[num_registered_fs] = fs;
	num_registered_fs++;
}