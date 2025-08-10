section .text

global insb
global insw
global outb
global outw

insb:
    push rbp
    mov rbp, rsp

    xor eax, eax
    mov dx, di
    in al, dx

    pop rbp
    ret

insw:
    push rbp
    mov rbp, rsp

    xor eax, eax
    mov dx, di
    in ax, dx

    pop rbp
    ret

outb:
    push rbp
    mov rbp, rsp

    mov dx, di
    mov al, sil
    out dx, al

    pop rbp
    ret

outw:
    push rbp
    mov rbp, rsp

    mov dx, di
    mov ax, si
    out dx, ax

    pop rbp
    ret
