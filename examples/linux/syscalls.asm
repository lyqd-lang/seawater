section .bss
    tmp_str: resb 1

section .text
global putc
putc:
    mov rax, [rsp + 8]
    mov [tmp_str], rax
    mov rax, 1
    mov rdi, 1
    mov rsi, tmp_str
    mov rdx, 1
    syscall
    ret

global exit
putc:
    mov rdi, [rsp + 8]
    mov rax, 60
    syscall
    ret

global puts ; TODO: Fix this function, not exactly sure where it's going wrong tho
puts:
    push rbp
    mov rbp, rsp
    mov rax, 1
    mov rdi, 1
    mov rsi, [rbp + 16]
    mov rdx, [rbp + 8]
    syscall
    mov rsp, rbp
    pop rbp
    ret