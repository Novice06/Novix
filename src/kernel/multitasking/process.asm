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

CR3_OFFSET  equ 0
ESP_OFFSET  equ 4

global task_switch
task_switch:

    mov eax, [esp+4]    ; previous task
    mov ecx, [esp+8]    ; next task

    ;Notes:
    ; For cdecl; EAX, ECX, and EDX are already saved by the caller and
    ; don't need to be saved again
    ; EIP is already saved on the stack by the caller's "CALL" instruction
    ; The task isn't able to change CR3 so it doesn't need to be saved
    ; Segment registers are constants (while running kernel code) so they
    ; don't need to be saved

    push ebx
    push esi
    push edi
    push ebp
    pushf

    mov ebx, [eax+CR3_OFFSET]   ; previous cr3
    mov edx, [ecx+CR3_OFFSET]   ; next cr3

    mov [eax+ESP_OFFSET], esp   ; save previous stack
    mov esp, [ecx+ESP_OFFSET]   ; load new stack

    cmp ebx, edx
    je .sameVAS

    mov cr3, edx

.sameVAS:
    popf
    pop ebp
    pop edi
    pop esi
    pop ebx
    ret

global switch_to_usermode
switch_to_usermode:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    mov ax, (4 * 8) | 3 ; ring 3 data with bottom 2 bits set for ring 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; set up the stack frame iret expects
    push (4 * 8) | 3    ; user SS
    mov eax, [ebp+8]
    push eax            ; ESP
    pushf               ; EFLAGS
    push (3 * 8) | 3    ; user CS
    mov eax, [ebp+12]
    push eax            ; EIP

    iret

    ; restore old call frame (never goes there)
    mov esp, ebp
    pop ebp
    ret