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

#include <mem_manager/virtmem_manager.h>
#include <mem_manager/vmalloc.h>
#include <mem_manager/heap.h>
#include <drivers/device.h>
#include <stdbool.h>
#include <string.h>
#include <memory.h>
#include <boot_info.h>
#include <stdio.h>
#include <drivers/framebuffer.h>

video_info_t info;

static int64_t read(uint8_t* buffer, uint32_t offset , size_t len, void* dev, uint32_t flags);
static int64_t write(const uint8_t *buffer, uint32_t offset, size_t len, void* dev, uint32_t flags);
static int ioctl(int request, void* arg);

void FRAMEBUFFER_init(video_info_t* boot_info)
{
    memcpy(&info, boot_info, sizeof(video_info_t));

    for(int i = 0; i < (info.height * info.pitch * info.bytes_per_pixel) / 0x1000; i++)
        VIRTMEM_mapPageCustom((void*)info.framebuffer + (i * 0x1000), (void*)0xf0000000 + (i * 0x1000), true);

    info.framebuffer = 0xf0000000;

    device_t* fb = kmalloc(sizeof(device_t));

    strcpy(fb->name, "fb");
    fb->priv = NULL;
    fb->read = read;
    fb->write = write;
    fb->ioctl = ioctl;

    add_device(fb);
}

int64_t read(uint8_t* buffer, uint32_t offset , size_t len, void* dev, uint32_t flags)
{
    uint32_t limit = info.pitch * info.height * info.bytes_per_pixel;
    if(offset+len > limit)
        len = limit - offset;

    memcpy(buffer, (void*)info.framebuffer, len);

    return len;
}

int64_t write(const uint8_t *buffer, uint32_t offset, size_t len, void* dev, uint32_t flags)
{
    uint32_t limit = info.pitch * info.height * info.bytes_per_pixel;
    if(offset+len > limit)
        len = limit - offset;

    memcpy((void*)info.framebuffer, buffer, len);

    return len;
}

int ioctl(int request, void* arg)
{
    switch (request)
    {
    case FB_GET_INFO:
        memcpy(arg, &info, sizeof(video_info_t));
        return 0;

    case FB_BLIT_RECT:
        surface_t* rect = (surface_t*)arg;
        if((rect->x + rect->width) > info.width)
            rect->width = info.width - rect->x;

        if((rect->y + rect->height) > info.height)
            rect->height = info.height - rect->y;

        for(int y = rect->y; y < rect->height; y++)
        {
            memcpy((void*)info.framebuffer + y * info.pitch + rect->x * info.width, rect->pixels, rect->width * info.bytes_per_pixel);
        }

        // memcpy((void*)info.framebuffer + rect->origin.y * info.pitch + rect->origin.x * info.width, rect->pixels, rect->width * rect->height);
        return 0;
    
    default:
        break;
    }

    return -1;
}