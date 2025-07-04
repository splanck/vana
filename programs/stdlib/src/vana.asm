[BITS 32]

; System call numbers used by the stdlib wrappers
%define VANA_SYS_SUM                       0
%define VANA_SYS_WRITE                     1
%define VANA_SYS_GETKEY                    2
%define VANA_SYS_PUTCHAR                   3
%define VANA_SYS_MALLOC                    4
%define VANA_SYS_FREE                      5
%define VANA_SYS_PROCESS_LOAD_START        6
%define VANA_SYS_SYSTEM                    7
%define VANA_SYS_PROCESS_GET_ARGUMENTS     8
%define VANA_SYS_EXIT                      9

section .asm

global print:function
global vana_sum:function
global vana_getkey:function
global vana_malloc:function
global vana_free:function
global vana_putchar:function
global vana_process_load_start:function
global vana_system:function
global vana_exit:function
global vana_process_get_arguments:function

; void print(const char* filename)
print:
    push ebp
    mov ebp, esp
    push dword[ebp+8]
    mov eax, VANA_SYS_WRITE
    int 0x80
    add esp, 4
    pop ebp
    ret

; int vana_sum(int a, int b)
vana_sum:
    push ebp
    mov ebp, esp
    mov eax, VANA_SYS_SUM
    push dword[ebp+12] ; b
    push dword[ebp+8] ; a
    int 0x80
    add esp, 8
    pop ebp
    ret

; int vana_getkey()
vana_getkey:
    push ebp
    mov ebp, esp
    mov eax, VANA_SYS_GETKEY
    int 0x80
    pop ebp
    ret

; void vana_putchar(char c)
vana_putchar:
    push ebp
    mov ebp, esp
    mov eax, VANA_SYS_PUTCHAR
    push dword [ebp+8] ; Variable "c"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void* vana_malloc(size_t size)
vana_malloc:
    push ebp
    mov ebp, esp
    mov eax, VANA_SYS_MALLOC
    push dword[ebp+8] ; Variable "size"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void vana_free(void* ptr)
vana_free:
    push ebp
    mov ebp, esp
    mov eax, VANA_SYS_FREE
    push dword[ebp+8] ; Variable "ptr"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void vana_process_load_start(const char* filename)
vana_process_load_start:
    push ebp
    mov ebp, esp
    mov eax, VANA_SYS_PROCESS_LOAD_START
    push dword[ebp+8] ; Variable "filename"
    int 0x80
    add esp, 4
    pop ebp
    ret

; int vana_system(struct command_argument* arguments)
vana_system:
    push ebp
    mov ebp, esp
    mov eax, VANA_SYS_SYSTEM
    push dword[ebp+8] ; Variable "arguments"
    int 0x80
    add esp, 4
    pop ebp
    ret


; void vana_process_get_arguments(struct process_arguments* arguments)
vana_process_get_arguments:
    push ebp
    mov ebp, esp
    mov eax, VANA_SYS_PROCESS_GET_ARGUMENTS
    push dword[ebp+8] ; Variable arguments
    int 0x80
    add esp, 4
    pop ebp
    ret

; void vana_exit(int status)
vana_exit:
    push ebp
    mov ebp, esp
    mov eax, VANA_SYS_EXIT
    push dword [ebp+8]
    int 0x80
    add esp, 4
    pop ebp
    ret
