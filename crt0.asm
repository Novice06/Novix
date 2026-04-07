[bits 32]

extern main
extern __sys_exit


global _start
_start:
    xor ebp, ebp

    call main

    push eax
    call __sys_exit

    ret ; never here