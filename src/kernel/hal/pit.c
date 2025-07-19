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

#include <hal/pit.h>
#include <hal/io.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define COUNTER0_PORT       0X40
#define COUNTER1_PORT       0X41
#define COUNTER2_PORT       0X42
#define CW_PORT             0X43

#define FREQUENCY   1000

typedef enum{
    PIT_ICW_BINARYCODED_DECIMAL = 0X01,

    PIT_ICW_MODE0               = 0X00,
    PIT_ICW_MODE1               = 0X02,
    PIT_ICW_MODE2               = 0X04,
    PIT_ICW_MODE3               = 0X06,
    PIT_ICW_MODE4               = 0X08,
    PIT_ICW_MODE5               = 0X0A,

    PIT_ICW_RL_COUNTER_LATCHED  = 0X00, // Counter value is latched into an internal control register at the time of the I/O write operation.
    PIT_ICW_RL_LSB              = 0X10, // Read or Load Least Significant Byte (LSB) only
    PIT_ICW_RL_MSB              = 0X20, // Read or Load Most Significant Byte (MSB) only
    PIT_ICW_RL_LSB_MSB          = 0X30, // Read or Load LSB first then MSB

    PIT_ICW_COUNTER0            = 0X00,
    PIT_ICW_COUNTER1            = 0X40,
    PIT_ICW_COUNTER2            = 0X80,
}PIT_ICW_BYTE;

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void PIT_initialize()
{
    uint32_t count = 1193180 / FREQUENCY;

    // configuring COUNTER 0 for irq0
    outb(CW_PORT, PIT_ICW_BINARYCODED_DECIMAL | PIT_ICW_MODE2 | PIT_ICW_RL_LSB_MSB | PIT_ICW_COUNTER0);

    // sending the least significant byte first
    outb(COUNTER0_PORT, (uint8_t)(count & 0xFF));

    // sending the most significant byte
    outb(COUNTER0_PORT, (uint8_t)((count >> 8) & 0xFF));


    /*
    * TODO:
    * configuring COUNTER 2 for PC speaker
    */
}
