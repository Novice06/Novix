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

#include "list.h"

typedef void* (*memAlloc)(size_t __size);
typedef void (*memFree)(void* __ptr);

memAlloc malloc;
memFree free;

void List_init(memAlloc mallocFunc, memFree freeFunc)
{
    malloc = mallocFunc;
    free = freeFunc;
}

struct listNode* create_newNode(void* payload)
{
    struct listNode* new = malloc(sizeof(struct listNode));

    if(!new)
        return new;

    new->payload = payload;
    new->next = NULL;
    new->prev = NULL;

    return new;
}

list_t* create_newList()
{
    list_t* new = malloc(sizeof(list_t));

    if(!new)
        return new;

    new->count = 0;
    new->root = NULL;

    return new;
}

int list_add(list_t* list, void* payload)
{
    struct listNode* node = create_newNode(payload);

    if(!node)
        return -1;

    if(list->count == 0)
    {
        list->root = node;
        list->count++;
        return 0;
    }

    struct listNode* current = list->root;
    while (current->next != NULL)
        current = current->next;
    
    node->prev = current;
    current->next = node;

    list->count++;

    return 0;
}

void* list_getAt(list_t* list, uint32_t index)
{
    if(index >= list->count)
        return NULL;

    struct listNode* current = list->root;
    
    for(int i = 0; i < index && current->next != NULL; i++)
        current = current->next;

    return current->payload;
}

void list_merge(list_t* dest, list_t* src)
{
    for(uint32_t i = 0; i < src->count; i++)
        list_add(dest, list_getAt(src, i));
}

void* list_removeAt(list_t* list, uint32_t index)
{
    if(index >= list->count)
        return NULL;

    struct listNode* current = list->root;
    
    for(int i = 0; i < index && current->next != NULL; i++)
        current = current->next;

    void* payload = current->payload;

    if(current == list->root)
    {
        list->root = list->root->next;
        if(list->root)
            list->root->prev = NULL;
    }
    else if(current->next == NULL)
    {
        current->prev->next = NULL;
    }
    else
    {
        current->prev->next = current->next;
        current->next->prev = current->prev;
    }

    free(current);
    list->count--;
    return payload;
}