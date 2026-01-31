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

typedef void* (*memAlloc)(size_t __size);
typedef void (*memFree)(void* __ptr);

struct listNode
{
    void* payload;
    struct listNode* next;
    struct listNode* prev;
    
};

typedef struct list {
    uint32_t count;
    struct listNode* root;
}list_t;

void List_init(memAlloc mallocFunc, memFree freeFunc);

list_t* create_newList();

int list_add(list_t* list, void* payload);

void* list_getAt(list_t* list, uint32_t index);
void* list_removeAt(list_t* list, uint32_t index);

void list_merge(list_t* dest, list_t* src);