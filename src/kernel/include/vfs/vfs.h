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

//============================================================================
//    INTERFACE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define VFS_MAX_PATH_LENGTH 256
#define VFS_MAX_FILENAME 64
#define MAX_OPEN_FILES 24

typedef enum
{
    VFS_O_RDONLY    = 1 << 0,   // read only
    VFS_O_WRONLY    = 1 << 1,   // write only
    VFS_O_RDWR      = 1 << 2,   // read / write
    VFS_O_NONBLOCK  = 1 << 3,   // non block operation 
} vfs_open_mode_t;

typedef enum
{
    VFS_OK         = 0,     /* Operation successful */
    VFS_ERROR      = -1,    /* Generic error */
    VFS_ENOENT     = -2,    /* File or directory not found */
    VFS_EEXIST     = -3,    /* File already exists */
    VFS_EACCESS    = -4,    /* Permission denied */
    VFS_EISDIR     = -9,    /* Is a directory */
    VFS_ENOTDIR    = -10,   /* Not a directory */
    VFS_ENFILE     = -11,   /* Too many open files */
    VFS_EBADF      = -12,   /* Invalid file descriptor */
    VFS_EAGAIN     = -13,   /* he operation cannot be completed right now Try again */
} vfs_error_t;

typedef enum
{
    VFS_SEEK_SET = 1 << 0,
    VFS_SEEK_CUR = 1 << 1,
    VFS_SEEK_END = 1 << 2,
} vfs_whence_t;


struct filesystem;
struct vnode;
struct vnodeops;

/*
 * Represents a mounted virtual file system.
 * This structure links together the mount point and the file system operations.
 */
typedef struct vfs
{
    struct vfs *next;           /* Pointer to the next mounted file system (used in a linked list) */
    struct filesystem *vfs_op;  /* Pointer to the file system operations (driver) associated with this mount */
    struct vnode *vnodecovered; /* The vnode that this file system is mounted over (i.e., the mount point) */
    void *vfs_data;             /* Private data used by the specific file system implementation */
} vfs_t;


/*
 * Describes a file system driver and the operations it supports.
 * Each registered file system should implement these functions.
 */
typedef struct filesystem
{
    char fs_name[VFS_MAX_FILENAME];                                 /* Name of the file system (e.g., "ramfs", "ext2") */
    int (*VFS_mount)(struct vfs* mountpoint);                       /* Function to mount the file system on a device */
    int (*VFS_unmount)(struct vfs* mountpoint);                     /* Function to unmount the file system */
    int (*get_root)(struct vfs* mountpoint, struct vnode** result); /* Get the root vnode of the mounted FS */
}filesystem_t;


typedef enum {
    VNODE_NONE        = 0,    // No special flag
    VNODE_ROOT        = 1 << 0, // This vnode is the root of a filesystem
    
    #if 0
    VNODE_EXECUTABLE  = 1 << 1, // The file is executable
    VNODE_MAPPED      = 1 << 2, // The file is memory-mapped
    VNODE_MODIFIED    = 1 << 3, // File has been modified
    VNODE_DELETED     = 1 << 4, // File has been deleted but still in use
    VNODE_DIRECTORY   = 1 << 5  // This vnode is a directory
    #endif  // I haven't found a use for these yet
} vnode_flags_t;

typedef enum {VNON, VREG, VDIR}vtype;
/*
 * Represents a file or directory within a mounted file system.
 * Abstracts the underlying inode and links to file system operations.
 */
typedef struct vnode
{
    //uint32_t flags;
    uint32_t ref_count;             /* Reference count for this vnode (used to manage lifetime) */
    vtype vnode_type;
    uint16_t flags;
    struct vfs *VFS_mountedhere;    /* If another file system is mounted here, pointer to it */
    struct vnodeops *vnode_op;      /* Pointer to the vnode operations supported by this file/directory */
    struct vfs *vnode_vfs;          /* The VFS this vnode belongs to */
    void *vnode_data;               /* File-system-specific data (usually an inode or similar) */
}vnode_t;

/*
 * Defines the operations that can be performed on a vnode.
 * These must be implemented by each file system.
 */
typedef struct vnodeops
{
    int64_t (*read)(struct vnode* node, void *buffer, size_t size, int64_t offset, uint32_t flags);
    int64_t (*write)(struct vnode* node, const void *buffer, size_t size, int64_t offset, uint32_t flags);
    int (*ioctl)(struct vnode* node, const int command, void* arg);
    int (*identify)(struct vnode* node, uint64_t* fileSize); // doesnt fill if NULL is passed

    /* Find a file/directory by name */
    int (*lookup)(struct vnode* node_dir, const char* name, struct vnode** result);
}vnodeops_t;


/*
 * Represents an open file in the VFS layer.
 * Maintains current file position and access mode.
 */
typedef struct file_descriptor
{
    struct vnode *vnode;    /* The vnode associated with this file */
    uint16_t mode;          /* Mode in which the file was opened (read, write, etc.) */
    uint64_t position;      /* Current position within the file (for reading/writing) */
} file_descriptor_t;

//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================

void VFS_init();
void VFS_register_new_filesystem(filesystem_t* fs);

int VFS_mount(const char *fs_name, const char *mount_point);
int VFS_unmount(const char *mount_point);

int VFS_open(const char *path, uint16_t mode);
int VFS_close(int fd);

int64_t VFS_read(int fd, void *buffer, size_t size);

#define VFS_FD_STDOUT   -1
#define VFS_FD_STDERR   -2
#define VFS_FD_DEBUG    -3
int64_t VFS_write(int fd, const void *buffer, size_t size);

int VFS_ioctl(int fd, int command, void* arg);
int VFS_identify(int fd, uint64_t* fileSize);
size_t VFS_seek(int fd, size_t offset, int whence);
int VFS_mkdir(const char* path, uint16_t mode);