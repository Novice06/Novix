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

#include <multitasking/process.h>

void lock_scheduler(uint32_t *saved_Eflags);
void unlock_scheduler(uint32_t *saved_Eflags);

typedef struct mutex
{
    bool locked;
    int locked_count;
    process_t* owner;
    process_t* first_waiting_list;
    process_t* last_waiting_list;
}mutex_t;

mutex_t* create_mutex();
void destroy_mutex(mutex_t* mut);

void acquire_mutex(mutex_t* mut);
void release_mutex(mutex_t* mut);