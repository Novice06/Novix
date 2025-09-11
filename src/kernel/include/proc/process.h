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

typedef enum{DEAD, RUNNING, READY, BLOCKED}status_t;

typedef struct process
{
    void* cr3;
    void* esp;
    void* esp0;
    bool usermode;
    void* entryPoint;
    uint64_t id;
    status_t state;
    struct process *next;
}process_t;

void lock_scheduler();
void unlock_scheduler();

void block_task();
void unblock_task(process_t* proc);

void yield();
process_t* PROCESS_getCurrent();
void PROCESS_initializeMultiTasking();
void PROCESS_createKernelProcess(void* task);
void PROCESS_terminate();

bool PROCESS_isMultitaskingEnabled();