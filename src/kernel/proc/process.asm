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
    mov ebx, [esp+8]   ; next task

    push ebx
    push esi
    push edi
    push ebp
    pushf

    mov ecx, [eax+CR3_OFFSET]   ; previous cr3
    mov edx, [ebx+CR3_OFFSET]   ; next cr3

    mov [eax+ESP_OFFSET], esp   ; save previous stack
    mov esp, [ebx+ESP_OFFSET]   ; load new stack

    cmp ecx, edx
    je .sameVAS

    mov cr3, edx

.sameVAS:
    popf
    pop ebp
    pop edi
    pop esi
    pop ebx
    ret