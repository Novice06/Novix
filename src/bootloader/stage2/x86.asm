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


%macro x86_EnterRealMode 0
    [bits 32]
    jmp word 18h:.pmode16         ; 1 - jump to 16-bit protected mode segment

.pmode16:
    [bits 16]
    ; 2 - disable protected mode bit in cr0
    mov eax, cr0
    and al, ~1
    mov cr0, eax

    ; 3 - jump to real mode
    jmp word 00h:.rmode

.rmode:
    ; 4 - setup segments
    mov ax, 0
    mov ds, ax
    mov ss, ax

    ; 5 - enable interrupts
    sti

%endmacro


%macro x86_EnterProtectedMode 0
    cli

    ; 4 - set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; 5 - far jump into protected mode
    jmp dword 08h:.pmode


.pmode:
    ; we are now in protected mode!
    [bits 32]
    
    ; 6 - setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax

%endmacro

; Convert linear address to segment:offset address
; Args:
;    1 - linear address
;    2 - (out) target segment (e.g. es)
;    3 - target 32-bit register to use (e.g. eax)
;    4 - target lower 16-bit half of #3 (e.g. ax)

%macro LinearToSegOffset 4

    mov %3, %1      ; linear address to eax
    shr %3, 4
    mov %2, %4
    mov %3, %1      ; linear address to eax
    and %3, 0xf

%endmacro


global x86_outb
x86_outb:
    [bits 32]
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global x86_inb
x86_inb:
    [bits 32]
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

global x86_Disk_CheckExtensions
x86_Disk_CheckExtensions:
    [bits 32]

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    x86_EnterRealMode

    [bits 16]

    ; save regs
    push es
    push bx
    push esi
    push di

    mov ah, 0x41
    mov dl, [bp+8]
    mov di, 0
    mov es, di
    mov bx, 0x55AA
    stc
    int 13h

    jc .no_disk_extensions
    cmp bx, 0xAA55
    jne .no_disk_extensions

    ; extensions are present
    mov eax, 1
    jmp .end

.no_disk_extensions:
    mov eax, 0

.end
    ; restore regs
    pop di
    pop esi
    pop bx
    pop es

    ; return

    push eax

    x86_EnterProtectedMode

    [bits 32]

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global x86_Disk_GetDriveParams
x86_Disk_GetDriveParams:
    [bits 32]

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    x86_EnterRealMode

    [bits 16]

    ; save regs
    push es
    push bx
    push esi
    push di

    ; call int13h
    mov dl, [bp + 8]    ; dl - disk drive
    mov ah, 08h
    mov di, 0           ; es:di - 0000:0000
    mov es, di
    stc
    int 13h

    ; out params
    mov eax, 1
    sbb eax, 0

    ; drive type from bl
    LinearToSegOffset [bp + 12], es, esi, si
    mov [es:si], bl

    ; cylinders
    mov bl, ch          ; cylinders - lower bits in ch
    mov bh, cl          ; cylinders - upper bits in cl (6-7)
    shr bh, 6
    inc bx

    LinearToSegOffset [bp + 16], es, esi, si
    mov [es:si], bx

    ; sectors
    xor ch, ch          ; sectors - lower 5 bits in cl
    and cl, 3Fh
    
    LinearToSegOffset [bp + 20], es, esi, si
    mov [es:si], cx

    ; heads
    mov cl, dh          ; heads - dh
    inc cx

    LinearToSegOffset [bp + 24], es, esi, si
    mov [es:si], cx

    ; restore regs
    pop di
    pop esi
    pop bx
    pop es

    ; return

    push eax

    x86_EnterProtectedMode

    [bits 32]

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret


global x86_Disk_Reset
x86_Disk_Reset:
    [bits 32]

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame


    x86_EnterRealMode

    mov ah, 0
    mov dl, [bp + 8]    ; dl - drive
    stc
    int 13h

    mov eax, 1
    sbb eax, 0           ; 1 on success, 0 on fail   

    push eax

    x86_EnterProtectedMode

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret


global x86_Disk_Read
x86_Disk_Read:

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    x86_EnterRealMode

    ; save modified regs
    push ebx
    push es

    ; setup args
    mov dl, [bp + 8]    ; dl - drive

    mov ch, [bp + 12]    ; ch - cylinder (lower 8 bits)
    mov cl, [bp + 13]    ; cl - cylinder to bits 6-7
    shl cl, 6
    
    mov al, [bp + 16]    ; cl - sector to bits 0-5
    and al, 3Fh
    or cl, al

    mov dh, [bp + 20]   ; dh - head

    mov al, [bp + 24]   ; al - count

    LinearToSegOffset [bp + 28], es, ebx, bx

    ; call int13h
    mov ah, 02h
    stc
    int 13h

    ; set return value
    mov eax, 1
    sbb eax, 0           ; 1 on success, 0 on fail   

    ; restore regs
    pop es
    pop ebx

    push eax

    x86_EnterProtectedMode

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global x86_Disk_ExtendedRead
x86_Disk_ExtendedRead:

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    x86_EnterRealMode

    ; save modified regs
    push ebx
    push esi
    push edi
    push es

    mov eax, [bp+12]    ;count
    mov [extensions_dap.count], eax

    LinearToSegOffset [bp + 24], es, esi, si
    mov [extensions_dap.offset], si
    mov [extensions_dap.segment], es

    mov eax, [bp+16]
    mov [extensions_dap.lba], eax
    mov eax, [bp+20]
    mov [extensions_dap.lba+4], eax


    ; call int13h
    mov ah, 42h
    mov dl, [bp+8]  ; drive number
    ; ds is already set
    mov si, extensions_dap
    stc
    int 13h

    ; set return value
    mov eax, 1
    sbb eax, 0           ; 1 on success, 0 on fail   

    ; restore regs
    pop es
    pop edi
    pop esi
    pop ebx

    push eax

    x86_EnterProtectedMode

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

extensions_dap:
    .size:              db 10h
                        db 0
    .count:             dw 0
    .offset:            dw 0
    .segment:           dw 0
    .lba:               dq 0

global x86_Get_MemorySize
x86_Get_MemorySize:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    ; cdecl convention must save all modified register except eax
    push ebx
    push ecx
    push edx

    x86_EnterRealMode

    xor ecx, ecx    ; very important for the check some bios might use eax and ebx instead of ecx and edx or both
    xor edx, edx

    mov eax, 0xE801
    int 15h
    jc .error

    cmp ah, 0x86 ; unsupported function
    je .error

    cmp ah, 0x80 ; invalid command
    je .error

    or ecx, ecx
    jz .inEAX   ;the bios chooses eax and ebx

    mov eax, edx
    mov edx, 64
    mul edx

    add eax, ecx

    ;mov ecx, 1024  ; uncomment if you want an output in Mb
    ;div ecx
    ;add eax, 1     ; the routine doesnt add the KB between 0-1MB; add it

    add eax, 1024

    jmp .done
    
.inEAX:
    xchg eax, ebx
    mov edx, 64
    mul edx

    add eax, ebx

    ;mov ecx, 1024  ; uncomment if you want an output in Mb
    ;div ecx
    ;add eax, 1     ; the routine doesnt add the KB between 0-1MB; add it

    add eax, 1024

    jmp .done

.error:
    mov eax, 0

.done:
    push eax
    x86_EnterProtectedMode
    pop eax


    pop edx
    pop ecx
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global x86_Get_MemoryMap
x86_Get_MemoryMap:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    push ebx
    push ecx
    push edx
    push ds
    push es
    push si
    push di

    x86_EnterRealMode

    LinearToSegOffset [bp+8], es, edi, di   ; Set DI register for memory storage

    xor ebx, ebx                ; EBX must be 0
    xor si, si                  ; SI must be 0 (to keep an entry count)

    mov edx, 0x534D4150         ; Place "SMAP" into edx | The "SMAP" signature ensures that the BIOS provides the correct memory map format
    mov eax, 0xe820             ; Function 0xE820 to get memory map
    mov ecx, 20                 ; Request 20 bytes of data

    int 0x15                    ; using interrupt

    jc short .failed            ; carry set on first call means "unsupported function"

    mov edx, 0x534D4150         ; Some BIOSes apparently trash this register? lets set it again
    cmp eax, edx                ; on success, eax must have been reset to "SMAP"
    jne short .failed

    test ebx, ebx               ; ebx = 0 implies list is only 1 entry long (worthless)
    je short .failed
    jmp short .jmpin
    
.e820lp:
    mov eax, 0xe820             ; eax, ecx get trashed on every int 0x15 call
    mov ecx, 20                 ; ask for 20 bytes again
    int 0x15
    jc short .done              ; carry set means "end of list already reached"
    mov edx, 0x534D4150         ; repair potentially trashed register

.jmpin:
    jcxz .skipent               ; skip any 0 length entries (If ecx is zero, skip this entry (indicates an invalid entry length))

.notext:
    mov eax, [es:di + 8]        ; get lower uint32_t of memory region length
    or eax, [es:di + 12]        ; "or" it with upper uint32_t to test for zero and form 64 bits (little endian)
    jz .skipent                 ; if length uint64_t is 0, skip entry
    inc si                      ; got a good entry: ++count, move to next storage spot
    add di, 20                  ; move next entry into buffer

.skipent:
    test ebx, ebx               ; if ebx resets to 0, list is complete
    jne short .e820lp
    jmp .done


.failed:
    mov eax, -1                 ; error code

.done:

    mov ebx, [bp+12]            ; memory entry count
    mov [ebx], si

    push eax
    x86_EnterProtectedMode
    pop eax

    pop di
    pop si
    pop es
    pop ds
    pop edx
    pop ecx
    pop ebx

; restore old call frame
    mov esp, ebp
    pop ebp
    ret