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
#include <drivers/device.h>
#include <drivers/vga_text.h>

static int64_t read(uint8_t* buffer, uint32_t offset , size_t len, void* dev);
static int64_t write(const uint8_t *buffer, uint32_t offset, size_t len, void* dev);

void create_console()
{
    device_t* console = kmalloc(sizeof(device_t));

    strcpy(console->name, "console");
    console->priv = NULL;
    console->read = read;
    console->write = write;

    add_device(console);
}

int64_t read(uint8_t* buffer, uint32_t offset , size_t len, void* dev)
{
    // keyboard
}

int64_t write(const uint8_t *buffer, uint32_t offset, size_t len, void* dev)
{
    for(size_t i = 0; i < len; i++)
        VGA_putc(buffer[i]);

    return len;
}