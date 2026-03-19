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
#include <multitasking/ipc/message.h>
#include <multitasking/ipc/shared_memory.h>
#include <drivers/keyboard.h>

typedef void (*SyscallHandler)(Registers* regs);

void SYSCALL_terminate(Registers* regs)
{
    PROCESS_terminate();
}

void SYSCALL_yield(Registers* regs)
{
    yield();
}

void SYSCALL_sleep(Registers* regs)
{
    sleep(regs->ebx);
}

void SYSCALL_openInbox(Registers* regs)
{
    open_inbox();
}

void SYSCALL_closeInbox(Registers* regs)
{
    close_inbox();
}

void SYSCALL_send_msg(Registers* regs)
{
    regs->edx = send_msg(regs->ebx, (void*)regs->esi, regs->ecx);
}

void SYSCALL_receive_async_msg(Registers* regs)
{
    uint32_t size;
    regs->edx = receive_async_msg((void*)regs->edi, &size);
    regs->ebx = size;
}

void SYSCALL_receive_msg(Registers* regs)
{
    uint32_t size;
    receive_msg((void*)regs->edi, &size);
    regs->ebx = size;
}

void SYSCALL_shm_create(Registers* regs)
{
    uint64_t id = shared_memory_create(regs->ebx);

    // edx:ecx
    regs->ecx = id;
    regs->edx = id >> 32;
}

void SYSCALL_shm_attach(Registers* regs)
{
    uint64_t id = regs->edx;
    id = (id << 32) | regs->ecx;
    regs->esi = (uint32_t)shared_memory_attach(id);
}

void SYSCALL_shm_detach(Registers* regs)
{
    uint64_t id = regs->edx;
    id = (id << 32) | regs->ecx;
    shared_memory_detach(id);
}

void SYSCALL_getId(Registers* regs)
{
    regs->ebx = PROCESS_getCurrent()->id;
}

void SYSCALL_open(Registers* regs)
{
    regs->edx = VFS_open((char*)regs->esi, regs->ebx);
}

void SYSCALL_read(Registers* regs)
{
    int64_t size = regs->edx;
    size = (size << 32) | regs->ecx;

    size = VFS_read(regs->ebx, (void*)regs->edi, size);

    // edx:ecx
    regs->ecx = size;
    regs->edx = size >> 32;
}

void SYSCALL_write(Registers* regs)
{
    int64_t size = regs->edx;
    size = (size << 32) | regs->ecx;

    size = VFS_write(regs->ebx, (void*)regs->esi, size);

    // edx:ecx
    regs->ecx = size;
    regs->edx = size >> 32;
}

void SYSCALL_close(Registers* regs)
{
    regs->edx = VFS_close(regs->ebx);
}

void SYSCALL_ioctl(Registers* regs)
{
    regs->edx = VFS_ioctl(regs->ebx, regs->ecx, (void*)regs->esi);
}

void SYSCALL_seek(Registers* regs)
{
    int64_t offset = regs->edx;
    offset = (offset << 32) | regs->ecx;

    regs->edx = VFS_seek(regs->ebx, offset, regs->ecx);
}

void SYSCALL_mkdir(Registers* regs)
{
    regs->edx = VFS_mkdir((char*)regs->esi, regs->ebx);
}

void SYSCALL_keyeventToAscii(Registers* regs)
{
    regs->ebx = KEYBOARD_scanToAscii((void*)regs->esi);
}

SyscallHandler Handlers[] = {
    [0]     = SYSCALL_terminate,
    [1]     = SYSCALL_yield,
    [2]     = SYSCALL_sleep,
    [3]     = SYSCALL_openInbox,
    [4]     = SYSCALL_closeInbox,
    [5]     = SYSCALL_send_msg,
    [6]     = SYSCALL_receive_async_msg,
    [7]     = SYSCALL_receive_msg,
    [8]     = SYSCALL_shm_create,
    [9]     = SYSCALL_shm_attach,
    [10]    = SYSCALL_shm_detach,
    [11]    = SYSCALL_getId,
    [12]    = SYSCALL_open,
    [13]    = SYSCALL_read,
    [14]    = SYSCALL_write,
    [15]    = SYSCALL_close,
    [16]    = SYSCALL_keyeventToAscii,
    [17]    = SYSCALL_ioctl,
    [18]    = SYSCALL_seek,
};

void SYSCALL_handler(Registers* regs)
{
    if(regs->eax > (sizeof(Handlers) / sizeof(SyscallHandler)) || Handlers[regs->eax] == NULL)
        return;

    Handlers[regs->eax](regs);
}

void SYSCALL_initialize()
{
    void __attribute__((cdecl)) ISR128();

    IDT_setGate(0x80, ISR128, IDT_ATTRIBUTE_32BIT_TRAP_GATE | IDT_ATTRIBUTE_DPL_RING3 | IDT_ATTRIBUTE_PRESENT_BIT);
    ISR_registerNewHandler(0x80, SYSCALL_handler);
}