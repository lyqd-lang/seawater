section .bss
    tmp_str: resb 1
    tmp_inp: resb 1024

section .text
global __x86_64_clyqd_fputc
__x86_64_clyqd_fputc:
    xor rax, rax
    mov al, byte [rsp + 16]
    mov [tmp_str], al
    mov rax, 1
    mov rdi, [rsp + 8]
    mov rsi, tmp_str
    mov rdx, 1
    syscall
    ret

global __x86_64_clyqd_exit
__x86_64_clyqd_exit:
    mov rdi, [rsp + 8]
    mov rax, 60
    syscall
    ret
