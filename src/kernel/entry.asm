; Copyright (C) 2025,  Novice
;
; This file is part of the Novix software.
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.


[bits 32]

PAGE_DIR        equ 0x80000 ; page directory table
PAGE_TABLE_0    equ 0x81000 ; 0th page table. Address must be 4KB aligned
PAGE_TABLES_FROM_768  equ 0x82000 ; 768th page table. Address must be 4KB aligned

PAGE_FLAGS      equ 0x03 ; attributes (page is present;page is writable; supervisor mode)
PAGE_ENTRIES    equ 1024 ; each page table has 1024 entries

section .stack nobits alloc noexec write align=4    ; the stack won't take space in the executable file
stack_bottom:
    resb 0x1000
stack_top:

section .entry
extern start
global entry

entry:
    mov edx, [esp+4]    ; boot_info struct from the bootloader

    ;------------------------------------------
	;	idenitity map 1st page table (4MB)
	;------------------------------------------

    mov eax, PAGE_TABLE_0           ; first page table
    mov ebx, 0x0 | PAGE_FLAGS       ; starting physical address of page
    mov ecx, PAGE_ENTRIES           ; for every page in table...

.loop1:
    mov dword [eax], ebx            ; write the entry
    add eax, 4                      ; go to next page entry in table (Each entry is 4 bytes)
    add ebx, 0x1000                 ; go to next page address (Each page is 4Kb)

    loop .loop1

    ;------------------------------------------
	;	map the from 768th to whatever table based on the kernel size
	;	to physical addr 1MB the 768th table starts the 3gb virtual address
	;------------------------------------------

    extern __end

    push edx    ; saving edx cause mul and div operation modifies it

    xor edx, edx

    ; computing the total number of page tables needed to map the kernel
    mov eax, __end
    sub eax, 0xc0000000 ; kernel size

    mov ebx, 0x400000
    div ebx

    or edx, edx
    jz .noRound

    inc eax ; rounding up, this is the number of page table we need to map for the kernel

.noRound:
    mov ebx, eax
    xor ecx, ecx    ; for every page table

    ; now we need too map all those page tables with valid pages
.loop2A:    ; loop for every page tables
    push ebx
    push ecx

    mov eax, ecx

    mov ebx, 0x400000
    mul ebx

    mov ebx, eax  
    add ebx, 0x100000 
    or ebx, PAGE_FLAGS              ; starting physical address of page

    mov eax, 0x1000
    mul ecx
    add eax, PAGE_TABLES_FROM_768   ; this page table

    mov ecx, PAGE_ENTRIES           ; for every page in table...

.loop2B:    ; loop for every pages in the page tables
    mov dword [eax], ebx            ; write the entry
    add eax, 4                      ; go to next page entry in table (Each entry is 4 bytes)
    add ebx, 0x1000                 ; go to next page address (Each page is 4Kb)

    loop .loop2B

    pop ecx
    pop ebx

    inc ecx

    cmp ecx, ebx
    jne .loop2A

    ;------------------------------------------
	;	set up the entries in the directory table
	;------------------------------------------

    mov eax, PAGE_TABLE_0 | PAGE_FLAGS      ; 1st table is directory entry 0
    mov dword [PAGE_DIR], eax

    xor ecx, ecx

.loop3: ; loop to register every page table in the page directory table
    mov eax, 0x1000
    mul ecx

    add eax, PAGE_TABLES_FROM_768

    or eax, PAGE_FLAGS      ; 768 + cx th entry in directory table

    push eax
    mov edx, 768
    add edx, ecx

    mov eax, 4
    mul edx
    mov edx, eax
    pop eax

    add edx, PAGE_DIR

    mov dword [edx], eax    ; setting this page directory entry

    inc ecx

    cmp ecx, ebx
    jne .loop3

    pop edx ; restoring edx
    
    ;------------------------------------------
	;	install directory table
	;------------------------------------------

	mov		eax, PAGE_DIR
	mov		cr3, eax

	;------------------------------------------
	;	enable paging
	;------------------------------------------

	mov		eax, cr0
	or		eax, 0x80000000
	mov		cr0, eax

    ;------------------------------------------
	; Now that paging is enabled, we can set up the stack
    ; and jump to the higher half address
    ;------------------------------------------
    mov esp, stack_top
    jmp higher_half

section .text
higher_half:

    push edx    ; boot_info struct from the bootloader
    call start

    cli
    hlt
