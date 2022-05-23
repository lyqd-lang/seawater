; Use C because idfk how this shit works

global __x86_64_clyqd_putc
extern putchar

section .text
__x86_64_clyqd_putc:
    mov rax, [rsp + 8]
    push rax
    call putchar
    ret