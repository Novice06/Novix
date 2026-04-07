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

#include <string.h>
#include <multitasking/lock.h>
#include <drivers/device.h>

list_t* devices;
mutex_t* dev_lock;

void init_device_manager()
{
    dev_lock = create_mutex();
    devices = create_newList();
}

void add_device(device_t* dev)
{
    acquire_mutex(dev_lock);
    list_add(devices, dev);
    release_mutex(dev_lock);
}

device_t* lookup_device(const char* name)
{
    acquire_mutex(dev_lock);

    for(uint32_t i = 0; i < devices->count; i++)
    {
        device_t* dev = (device_t*)list_getAt(devices, i);
        if(strcmp(name, dev->name) == 0)
        {
            release_mutex(dev_lock);
            return dev;
        }
            
    }

    release_mutex(dev_lock);
    return NULL;
}

int get_device_list(char* buff, uint32_t count)
{
    uint32_t i;

    acquire_mutex(dev_lock);

    for(i = 0; i < devices->count && i < count; i++)
    {
        device_t* dev = (device_t*)list_getAt(devices, i);
        strncpy(buff + (i * MAX_NAME_LENGTH), dev->name, MAX_NAME_LENGTH);
    }

    release_mutex(dev_lock);
    
    return i;
}