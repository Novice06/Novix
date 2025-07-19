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

//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================

void PIC_configure(uint8_t offsetPic1, uint8_t offsetPic2);
void PIC_sendEndOfInterrupt(int irq);
void PIC_disable();
void PIC_mask(int irq);
void PIC_unMask(int irq);
uint16_t PIC_readIrqRequestRegister();
uint16_t PIC_readInServiceRegister();