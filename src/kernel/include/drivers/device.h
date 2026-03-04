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
#include <list.h>

#define MAX_NAME_LENGTH 64

typedef struct device
{
	char name[MAX_NAME_LENGTH];

	// functions to interact with the device
	int64_t (*read)(uint8_t* buffer, uint32_t offset , size_t len, void* dev, uint32_t flags);
	int64_t (*write)(const uint8_t *buffer, uint32_t offset, size_t len, void* dev, uint32_t flags);

	void *priv;	// private data of the device ...
} device_t;

void init_device_manager();
void add_device(device_t* dev);
device_t* lookup_device(const char* name);