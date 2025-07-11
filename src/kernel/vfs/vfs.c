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
#include <vfs/vfs.h>
#include <drivers/e9_port.h>
#include <drivers/vga_text.h>

size_t VFS_write(fd_t fd, const void *buffer, size_t size)
{
	switch (fd)
    {
    case VFS_FD_STDOUT:
    case VFS_FD_STDERR:
        for (size_t i = 0; i < size; i++)
            VGA_putc(*((uint8_t*)buffer+i));
        return size;

    case VFS_FD_DEBUG:
        for (size_t i = 0; i < size; i++)
			E9_putc((*(uint8_t*)buffer+i));
        return size;
    }
}