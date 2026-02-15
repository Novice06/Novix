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

/* Ramfs simulates an in-memory file system with an N-ary tree, perfect for initial testing */

typedef enum {
    NODE_FILE,
    NODE_DIRECTORY
} nodetype_t;

// Metadata structure for files and directories
typedef struct metadata {
    char name[VFS_MAX_FILENAME];
    nodetype_t type;
    uint64_t size;
    // uint64_t create_time;
    // uint64_t modify_time;
    // uint64_t access_time;
} metadata_t;

// Structure of a node in the N-ary tree
typedef struct treenode {
    metadata_t meta;
    struct treenode *parent;
    struct treenode *first_child;
    struct treenode *next_sibling;
    uint8_t *data; // file content
} treenode_t;

void ramfs_init();