[bits 32]

global __sys_open
__sys_open:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame
    
    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 12
    mov esi, [ebp+8]
    mov ebx, [ebp+12]
    int 0x80

    mov eax, edx

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_read
__sys_read:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 13
    mov ecx, [ebp + 16]     ; low size
    mov edx, [ebp + 20]     ; high size
    mov ebx, [ebp + 8]      ; fd
    mov edi, [ebp + 12]      ; buffer
    int 0x80

    ; the syscall already puts the higher size in edx
    mov eax, ecx    ;return value edx:eax

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_write
__sys_write:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 14
    mov ecx, [ebp + 16]     ; low size
    mov edx, [ebp + 20]     ; high size
    mov ebx, [ebp + 8]      ; fd
    mov esi, [ebp + 12]      ; buffer
    int 0x80

    ; the syscall already puts the higher size in edx
    mov eax, ecx    ;return value edx:eax

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_close
__sys_close:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 15
    mov ebx, [ebp+8]
    int 0x80

    mov eax, edx

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_exit
__sys_exit:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 0
    int 0x80

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_sbrk
__sys_sbrk:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 25
    mov ebx, [ebp + 8]     ; size
    int 0x80

    mov eax, esi

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_getpid
__sys_getpid:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 11
    int 0x80

    mov eax, ebx

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_execve
__sys_execve:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 24
    mov esi, [ebp+8]
    mov ebx, [ebp+12]
    int 0x80

    mov eax, edx

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_seek
__sys_seek:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 18
    mov ebx, [ebp+8]    ; fd
    mov ecx, [ebp+12]   ; low off
    mov edx, [ebp+16]   ; high off
    mov edi, [ebp+20]   ; whence
    int 0x80

    ; the syscall already puts the higher size in edx
    mov eax, ecx    ;return value edx:eax

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_ioctl
__sys_ioctl:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 17
    mov ebx, [ebp+8]
    mov ecx, [ebp+12]
    mov esi, [ebp+16]
    int 0x80

    mov eax, edx

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_mkdir
__sys_mkdir:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 19
    mov esi, [ebp+8]
    mov ebx, [ebp+12]
    int 0x80

    mov eax, edx

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_rmdir
__sys_rmdir:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 20
    mov esi, [ebp+8]
    int 0x80

    mov eax, edx

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_unlink
__sys_unlink:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 21
    mov esi, [ebp+8]
    int 0x80

    mov eax, edx

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_stat
__sys_stat:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 22
    mov esi, [ebp+8]
    mov edi, [ebp+12]
    int 0x80

    mov eax, edx

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global __sys_getdents
__sys_getdents:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; calle-save
    push ebx
    push esi
    push edi

    mov eax, 23
    mov esi, [ebp+8]
    mov edi, [ebp+12]
    mov ebx, [ebp+16]
    int 0x80

    mov eax, edx

    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret