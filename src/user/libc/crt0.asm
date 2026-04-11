[bits 32]

extern main
extern __sys_exit
extern init_streams


global _start
_start:
    xor ebp, ebp

    call init_libc
    call main

    push eax
    call __sys_exit

    ret ; never here

init_libc:
    ; mov rdi, rdx
    ; call init_environ
    call init_streams
    ret