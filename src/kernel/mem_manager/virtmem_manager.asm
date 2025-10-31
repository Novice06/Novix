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

global enablePaging
enablePaging:
    mov		eax, cr0
	or		eax, 0x80000000
	mov		cr0, eax
    ret

global flushTLB
flushTLB:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame
    
    pushf
    cli
    invlpg [ebp+8]
    sti
    popf

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global getPDBR
getPDBR:
    mov eax, cr3
    ret

global switchPDBR
switchPDBR:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    mov eax, [ebp+8]
    mov cr3, eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret
