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

#include <hal/isr.h>
#include <hal/io.h>
#include <stdio.h>
#include <stddef.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

static const char* const g_Exceptions[] = {
    "Divide by zero error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception ",
    "",
    "",
    "",
    "",
    "",
    "",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    ""
};

ISRHandler g_ISR_handlers[256];

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================

void ISR_initializeGates();
void ISR_handler(Registers* regs);

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

void ISR_handler(Registers* regs)
{
    if(g_ISR_handlers[regs->interrupt] != NULL)
        g_ISR_handlers[regs->interrupt](regs);

    else if (regs->interrupt >= 32)
        printf("Unhandled interrupt %d!\n", regs->interrupt);
    
    else {
        printf("Unhandled exception %d %s\n", regs->interrupt, g_Exceptions[regs->interrupt]);
        
        printf("  eax=%x  ebx=%x  ecx=%x  edx=%x  esi=%x  edi=%x\n",
               regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);

        printf("  esp=%x  ebp=%x  eip=%x  eflags=%x  cs=%x  ds=%x  ss=%x\n",
               regs->esp, regs->ebp, regs->eip, regs->eflags, regs->cs, regs->ds, regs->ss);

        printf("  interrupt=%x  errorcode=%x\n", regs->interrupt, regs->error);

        puts("KERNEL PANIC!\n");
        panic();
    }
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void ISR_initialze()
{
    ISR_initializeGates();
}

void ISR_registerNewHandler(int interrupt, ISRHandler handler)
{
    g_ISR_handlers[interrupt] = handler;
}