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
#include <stdbool.h>

typedef enum{DEAD, RUNNING, READY, BLOCKED, WAITING}status_t;

typedef struct process
{
    void* cr3;      // physical address of the page directory
    void* esp;      // current address of the stack

    void* esp0;     // kernel mode stack address
    void* virt_cr3; // virtual address of the page directory

    bool usermode;  // usermode or kernel process 
    uint32_t id;
    void* entryPoint;

    status_t state;
    struct process *next;
} __attribute__((packed)) process_t;

void __attribute__((cdecl)) task_switch(process_t *previous, process_t *next);
void __attribute__((cdecl)) switch_to_usermode(uint32_t stack, uint32_t ip);

void PROCESS_initialize(process_t* idle);
void PROCESS_createFrom(void* entryPoint);
void PROCESS_createFromByteArray(void* array, int length, bool is_usermode);
void PROCESS_terminate();

void block_task();
void unblock_task(process_t* proc, bool priority);