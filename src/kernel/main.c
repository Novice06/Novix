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

#include <boot_info.h>
#include <debug.h>
#include <stdint.h>
#include <utility.h>
#include <string.h>
#include <list.h>
#include <memory.h>
#include <hal/hal.h>
#include <hal/io.h>
#include <mem_manager/physmem_manager.h>
#include <mem_manager/virtmem_manager.h>
#include <mem_manager/heap.h>
#include <mem_manager/vmalloc.h>
#include <syscall/syscall.h>
#include <multitasking/scheduler.h>
#include <multitasking/process.h>
#include <multitasking/time.h>
#include <multitasking/lock.h>
#include <multitasking/ipc/message.h>
#include <multitasking/ipc/shared_memory.h>

const char logo[] = 
"\
             __    __   ______   __     __  ______  __    __ \n\
            |  \\  |  \\ /      \\ |  \\   |  \\|      \\|  \\  |  \\\n\
            | $$\\ | $$|  $$$$$$\\| $$   | $$ \\$$$$$$| $$  | $$\n\
            | $$$\\| $$| $$  | $$| $$   | $$  | $$   \\$$\\/  $$\n\
            | $$$$\\ $$| $$  | $$ \\$$\\ /  $$  | $$    >$$  $$ \n\
            | $$\\$$ $$| $$  | $$  \\$$\\  $$   | $$   /  $$$$\\ \n\
            | $$ \\$$$$| $$__/ $$   \\$$ $$   _| $$_ |  $$ \\$$\\\n\
            | $$  \\$$$ \\$$    $$    \\$$$   |   $$ \\| $$  | $$\n\
             \\$$   \\$$  \\$$$$$$      \\$     \\$$$$$$ \\$$   \\$$\n\n\
";

extern uint8_t __text_start;
extern uint8_t __end;

extern uint8_t __bss_start;
extern uint8_t __bss_end;

/*

org 0x400000
bits 32
main:

    mov eax, 3  ; open inbox to be able to receive messages
    int 0x80

    mov eax, 8	; request a shared memory
    mov ebx, 1  ; request 1Mb resulting in a 4k block
    int 0x80

    ; now we have the id of shared memory in edx:ecx
    mov [shared_id], ecx
    mov [shared_id+4], edx

    mov eax, 9	; attaching the shared memory
    int 0x80

    or esi, esi
    jnz .attached_successfully

    call exit   ; failed

.attached_successfully:
    ; now we have the base addr of the shared memory in esi
    mov [base], esi

    mov eax, 5          ; send message
    mov ebx, 3          ; to the process with id 3
    mov ecx, 8          ; the message is 8bytes long
    mov esi, shared_id  ; esi has the message (the shared id)
    int 0x80    

    or edx, edx
    jz .sent_successfully
    
    call exit   ; failed

.sent_successfully:
    mov eax, 12 ; get id of the current process
    int 0x80

    ; now ebx has the id
    mov [myid], ebx

    mov eax, 5          ; send message
    mov ebx, 3          ; to the process with id 3
    mov ecx, 4          ; the message is 4bytes long
    mov esi, myid       ; esi has the message (the my id)
    int 0x80

    or edx, edx
    jz .cpy_msg
    
    call exit   ; failed

.cpy_msg:

    mov edi, [base]
    mov esi, string

.cpy_msg_loop:
    mov al, [esi]
    mov [edi], al

    inc edi
    inc esi

    or al, al
    jnz .cpy_msg_loop

    ; sending a signal that we finished writing!
    mov eax, 5          ; send message
    mov ebx, 3          ; to the process with id 3
    mov ecx, 0          ; the message is 0bytes long
    mov esi, 0          ; NULL addresse because we are'nt sending anything
    int 0x80

    ; waiting for the process we're communicating with
    mov eax, 7          ; wait for message
    mov edi, 0          ; NULL we're not trying to read anything
    mov ebx, 0          ; NULL again
    int 0x80

    mov edi, [base]
    mov esi, [checksum]
    mov [edi], esi

    ; sending a signal that we finished writing!
    mov eax, 5          ; send message
    mov ebx, 3          ; to the process with id 3
    mov ecx, 0          ; the message is 0bytes long
    mov esi, 0          ; NULL addresse because we are'nt sending anything
    int 0x80

    ; the OS will detach the shared memory for us

    call exit

exit:
    mov eax, 0x0
    int 0x80

    ret

string db "Hello I'm a USER program don't trust me? here is a checksum: ", 0
checksum dd 0xDEADC0DE
shared_id dq 0
myid dd 0
base dd 0

*/

unsigned char user_bin[] = {
  0xb8, 0x03, 0x00, 0x00, 0x00, 0xcd, 0x80, 0xb8, 0x08, 0x00, 0x00, 0x00,
  0xbb, 0x01, 0x00, 0x00, 0x00, 0xcd, 0x80, 0x89, 0x0d, 0x2f, 0x01, 0x40,
  0x00, 0x89, 0x15, 0x33, 0x01, 0x40, 0x00, 0xb8, 0x09, 0x00, 0x00, 0x00,
  0xcd, 0x80, 0x09, 0xf6, 0x75, 0x05, 0xe8, 0xb6, 0x00, 0x00, 0x00, 0x89,
  0x35, 0x3b, 0x01, 0x40, 0x00, 0xb8, 0x05, 0x00, 0x00, 0x00, 0xbb, 0x03,
  0x00, 0x00, 0x00, 0xb9, 0x08, 0x00, 0x00, 0x00, 0xbe, 0x2f, 0x01, 0x40,
  0x00, 0xcd, 0x80, 0x09, 0xd2, 0x74, 0x05, 0xe8, 0x91, 0x00, 0x00, 0x00,
  0xb8, 0x0c, 0x00, 0x00, 0x00, 0xcd, 0x80, 0x89, 0x1d, 0x37, 0x01, 0x40,
  0x00, 0xb8, 0x05, 0x00, 0x00, 0x00, 0xbb, 0x03, 0x00, 0x00, 0x00, 0xb9,
  0x04, 0x00, 0x00, 0x00, 0xbe, 0x37, 0x01, 0x40, 0x00, 0xcd, 0x80, 0x09,
  0xd2, 0x74, 0x05, 0xe8, 0x65, 0x00, 0x00, 0x00, 0x8b, 0x3d, 0x3b, 0x01,
  0x40, 0x00, 0xbe, 0xed, 0x00, 0x40, 0x00, 0x8a, 0x06, 0x88, 0x07, 0x47,
  0x46, 0x08, 0xc0, 0x75, 0xf6, 0xb8, 0x05, 0x00, 0x00, 0x00, 0xbb, 0x03,
  0x00, 0x00, 0x00, 0xb9, 0x00, 0x00, 0x00, 0x00, 0xbe, 0x00, 0x00, 0x00,
  0x00, 0xcd, 0x80, 0xb8, 0x07, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x00, 0x00,
  0x00, 0xbb, 0x00, 0x00, 0x00, 0x00, 0xcd, 0x80, 0x8b, 0x3d, 0x3b, 0x01,
  0x40, 0x00, 0x8b, 0x35, 0x2b, 0x01, 0x40, 0x00, 0x89, 0x37, 0xb8, 0x05,
  0x00, 0x00, 0x00, 0xbb, 0x03, 0x00, 0x00, 0x00, 0xb9, 0x00, 0x00, 0x00,
  0x00, 0xbe, 0x00, 0x00, 0x00, 0x00, 0xcd, 0x80, 0xe8, 0x00, 0x00, 0x00,
  0x00, 0xb8, 0x00, 0x00, 0x00, 0x00, 0xcd, 0x80, 0xc3, 0x48, 0x65, 0x6c,
  0x6c, 0x6f, 0x20, 0x49, 0x27, 0x6d, 0x20, 0x61, 0x20, 0x55, 0x53, 0x45,
  0x52, 0x20, 0x70, 0x72, 0x6f, 0x67, 0x72, 0x61, 0x6d, 0x20, 0x64, 0x6f,
  0x6e, 0x27, 0x74, 0x20, 0x74, 0x72, 0x75, 0x73, 0x74, 0x20, 0x6d, 0x65,
  0x3f, 0x20, 0x68, 0x65, 0x72, 0x65, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20,
  0x63, 0x68, 0x65, 0x63, 0x6b, 0x73, 0x75, 0x6d, 0x3a, 0x20, 0x00, 0xde,
  0xc0, 0xad, 0xde, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
unsigned int user_bin_len = 319;

void taskA()
{
    uint8_t data[MAX_MESSAGE_SIZE];
    size_t size;
    uint32_t his_id;

    open_inbox();
    PROCESS_createFromByteArray(user_bin, user_bin_len, true);

    log_debug("taskA", "Ok What ?");
    receive_msg(data, &size);   // waiting for shared memory id

    log_debug("taskA", "Ok What ?");
    uint32_t* base = shared_memory_attach(*(uint64_t*)data);
    printf("attached successfully at: 0x%x\n", base);

    receive_msg(data, &size);   // waiting id of the process we're communicating with
    his_id = *(uint32_t*)data;

    receive_msg(NULL, NULL);    // waiting for process to finish writing

    puts((char*)base);

    send_msg(his_id, NULL, 0);  // we signal that we finished reading

    receive_msg(NULL, NULL);    // waiting for process to finish writing

    printf("0x%x", *base);

    PROCESS_terminate();
}

void init_process()
{
    SYSCALL_initialize();

    PROCESS_createFrom(taskA);
    // PROCESS_createFrom(taskB);
    // PROCESS_createFrom(taskC);
    // PROCESS_createFromByteArray(user_bin, user_bin_len, true);

    PROCESS_terminate();
}

void __attribute__((cdecl)) start(Boot_info* info)
{
    // calculate the kernel size and memset the bss section
    uint32_t kernel_size = ((uint32_t)(&__end) - 0xc0000000 + 0x100000) - 0x100000;
    memset(&__bss_start, 0, (&__bss_end) - (&__bss_start));

    log_info("kernel", "the Kernel is running");

    log_info("kernel", "kernel start 0x%x, kernel end 0x%x", &__text_start, &__end);
    log_info("kernel", "kernel size %d Kb", roundUp_div(kernel_size, 1024));

    puts(logo);

    HAL_initialize();
    
    PHYSMEM_initialize(info, kernel_size);
    VIRTMEM_initialize(kernel_size);
    HEAP_initialize();
    VMALLOC_initialize();

    List_init(kmalloc, kfree);

    SCHEDULER_initialize();
    PROCESS_createFrom(init_process);

    for(;;)
    {
        //yield();
        HLT();
    }
}