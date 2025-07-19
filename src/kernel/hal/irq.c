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

#include <stdio.h>
#include <stddef.h>
#include <hal/irq.h>
#include <hal/io.h>
#include <hal/pic.h>
#include <hal/pit.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define PIC_REMAP_OFFSET 0x20

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

IRQHandler g_IRQ_handlers[16];

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

void IRQ_handler(Registers* regs)
{
    int irq = regs->interrupt - PIC_REMAP_OFFSET;

    if (g_IRQ_handlers[irq] != NULL)
    {
        // handle IRQ
        g_IRQ_handlers[irq](regs);
    }
    else
    {
        uint8_t pic_isr = PIC_readInServiceRegister();
        uint8_t pic_irr = PIC_readIrqRequestRegister();
        printf("Unhandled IRQ %d  ISR=%x  IRR=%x...\n", irq, pic_isr, pic_irr);

        // send EOI
        PIC_sendEndOfInterrupt(irq);
    }
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void timer()
{
    PIC_sendEndOfInterrupt(0);
}

void IRQ_initialize()
{
    PIC_configure(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);
    PIT_initialize();

    for(int i = 0; i < 16; i++)
        ISR_registerNewHandler(PIC_REMAP_OFFSET + i, IRQ_handler);

    // pit
    IRQ_registerNewHandler(0, timer);

    // enable interrupts
    enableInterrupts();
}

void IRQ_registerNewHandler(int irq, IRQHandler handler)
{
    g_IRQ_handlers[irq] = handler;
}