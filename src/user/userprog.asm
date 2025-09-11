org 0x400000
bits 32
main:
    mov eax, 0x1
    mov ebx, string

    int 0x80    ; syscall

    jmp $

string db "I'm a usermode program !", 0