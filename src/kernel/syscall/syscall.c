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
#include <stdio.h>
#include <hal/isr.h>
#include <hal/idt.h>
#include <hal/io.h>
#include <multitasking/scheduler.h>
#include <multitasking/process.h>
#include <multitasking/time.h>

void SYSCALL_handler(Registers* regs)
{
    // this is bad we should make a table of function pointer not a switch

    switch (regs->eax)
    {
    case 0:
        PROCESS_terminate();
        break;
    
    case 1:
        printf("%s", (uint8_t*)regs->ebx);
        break;

    case 2:
        yield();
        break;

    case 3:
        sleep(regs->ebx);
        break;

    default:
        break;
    }
}

void SYSCALL_initialize()
{
    void __attribute__((cdecl)) ISR128();

    IDT_setGate(0x80, ISR128, IDT_ATTRIBUTE_32BIT_TRAP_GATE | IDT_ATTRIBUTE_DPL_RING3 | IDT_ATTRIBUTE_PRESENT_BIT);
    ISR_registerNewHandler(0x80, SYSCALL_handler);
}