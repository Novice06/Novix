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

global outb
outb:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global inb
inb:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

global panic
panic:
    cli
    hlt

global enableInterrupts
enableInterrupts:
    sti
    ret

global disableInterrupts
disableInterrupts:
    cli
    ret

global HLT
HLT:
    hlt
    ret

global get_eflags
get_eflags:
    pushf
    pop eax
    ret