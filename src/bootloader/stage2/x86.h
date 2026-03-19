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
#include <boot_info.h>

#include "vbe.h"

void __attribute__((cdecl)) x86_outb(uint16_t port, uint8_t value);
uint8_t __attribute__((cdecl)) x86_inb(uint16_t port);

bool __attribute__((cdecl)) x86_Disk_CheckExtensions (uint8_t drive);

bool __attribute__((cdecl)) x86_Disk_GetDriveParams(uint8_t drive,
                                                    uint8_t* driveTypeOut,
                                                    uint16_t* cylindersOut,
                                                    uint16_t* sectorsOut,
                                                    uint16_t* headsOut);

bool __attribute__((cdecl)) x86_Disk_Reset(uint8_t drive);

bool __attribute__((cdecl)) x86_Disk_Read(uint8_t drive,
                                          uint16_t cylinder,
                                          uint16_t sector,
                                          uint16_t head,
                                          uint8_t count,
                                          void* lowerDataOut);

bool __attribute__((cdecl)) x86_Disk_ExtendedRead(uint8_t drive, uint16_t count, uint64_t lba, void* lowerDataOut);

uint32_t __attribute__((cdecl)) x86_Get_MemorySize();
uint32_t __attribute__((cdecl)) x86_Get_MemoryMap(Memory_mapEntry* entries, uint32_t* memoryBlockCount);

bool __attribute__((cdecl)) x86_VESA_GetInfo(VbeInfoBlock_t* buff);
bool __attribute__((cdecl)) x86_VESA_GetModeInfo(vbe_mode_info_t* buff, uint16_t mode);
bool __attribute__((cdecl)) x86_VESA_SetMode(uint16_t mode, VbeInfoBlock_t* CtrlBlock);