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

#include <stdint.h>

void i686_PIC_configure(uint8_t offsetPic1, uint8_t offsetPic2);
void i686_PIC_sendEndOfInterrupt(int irq);
void i686_PIC_disable();
void i686_PIC_mask(int irq);
void i686_PIC_unMask(int irq);
uint16_t i686_PIC_readIrqRequestRegister();
uint16_t i686_PIC_readInServiceRegister();