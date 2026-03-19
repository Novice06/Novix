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

#include "vbe.h"
#include "x86.h"
#include "stdio.h"
#include <memory.h>

int diff(int x, int y)
{
    if(x > y) return x - y;

    return y - x;
}

bool init_graphics(int desired_width, int desired_height, video_info_t* videoInfo)
{
    VbeInfoBlock_t info;
    memset(&info, 0, sizeof(VbeInfoBlock_t));
    info.VbeSignature[0] = 'V';
    info.VbeSignature[1] = 'B';
    info.VbeSignature[2] = 'E';
    info.VbeSignature[3] = '2';

    printf("please works...\n");
    if(!x86_VESA_GetInfo(&info))
    {
        printf("not supported :(\n");
        return false;
    }

    printf("I dont have enough info !!\n");
    // for(;;);

    uint16_t offset  = info.VideoModePtr[0];
    uint16_t segment = info.VideoModePtr[1];

    uint16_t* modeptr = (uint16_t*)((segment << 4) + offset);

    uint16_t chosen_mode = 0;
    vbe_mode_info_t chosen_mode_info;
    int current_width = 0;
    int current_height = 0;
    for(int i = 0; modeptr[i] != 0xFFFF; i++)
    {
        vbe_mode_info_t mode_info;
        x86_VESA_GetModeInfo(&mode_info, modeptr[i]);

        if(!(mode_info.attributes & 0x80) || mode_info.bpp != 32 || (mode_info.memory_model != 4 && mode_info.memory_model != 6) /*Check if this is a packed pixel or direct color mode*/)
            continue;

        int diff1 = diff(desired_height * desired_width, current_width * current_height);
        int diff2 = diff(desired_height * desired_width, mode_info.width * mode_info.height);

        if(diff2 < diff1)
        {
            chosen_mode = modeptr[i];
            current_height = mode_info.height;
            current_width = mode_info.width;
            memcpy(&chosen_mode_info, &mode_info, sizeof(vbe_mode_info_t));
        }
    }

    if(current_width == 0 || current_height == 0)
    {
        debugf("No mode with Linear framebuffer, 32bits pixels and packed pixel or direct color mode were found !!");
        return false;
    }

    printf("Im sure this funcction causes problems\n");
    if(!x86_VESA_SetMode(chosen_mode, &info))
    {
        debugf("failed to set mode: %d, %d x %d\n", chosen_mode, current_width, current_height);
        return false;
    }

    videoInfo->pitch = chosen_mode_info.pitch;
    videoInfo->width = chosen_mode_info.width;
    videoInfo->height = chosen_mode_info.height;
    videoInfo->bpp = chosen_mode_info.bpp;
    videoInfo->bytes_per_pixel = videoInfo->bpp / 8;

    videoInfo->memory_model  = chosen_mode_info.memory_model;

    videoInfo->red_mask = chosen_mode_info.red_mask;
    videoInfo->red_position = chosen_mode_info.red_position;
    videoInfo->green_mask = chosen_mode_info.green_mask;
    videoInfo->green_position = chosen_mode_info.green_position;
    videoInfo->blue_mask = chosen_mode_info.blue_mask;
    videoInfo->blue_position = chosen_mode_info.blue_position;

    videoInfo->framebuffer = chosen_mode_info.framebuffer;
    videoInfo->off_screen_mem_off = chosen_mode_info.off_screen_mem_off;
    videoInfo->off_screen_mem_size = chosen_mode_info.off_screen_mem_size;

    return true;
}

