section .bss
    tmp_str: resb 1

section .text
global __x86_64_clyqd_putc
__x86_64_clyqd_putc:
    mov rax, [rsp + 8]
    mov [tmp_str], rax
    mov rax, 1
    mov rdi, 1
    mov rsi, tmp_str
    mov rdx, 1
    syscall
    ret

global exit
exit:
    mov rdi, [rsp + 8]
    mov rax, 60
    syscall
    ret
