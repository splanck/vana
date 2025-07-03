[BITS 32]

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
    mov eax, 1 ; Command print
    int 0x80
    add esp, 4
    pop ebp
    ret

; int vana_sum(int a, int b)
vana_sum:
    push ebp
    mov ebp, esp
    mov eax, 0 ; Command sum
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
    mov eax, 2 ; Command getkey
    int 0x80
    pop ebp
    ret

; void vana_putchar(char c)
vana_putchar:
    push ebp
    mov ebp, esp
    mov eax, 3 ; Command putchar
    push dword [ebp+8] ; Variable "c"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void* vana_malloc(size_t size)
vana_malloc:
    push ebp
    mov ebp, esp
    mov eax, 4 ; Command malloc (Allocates memory for the process)
    push dword[ebp+8] ; Variable "size"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void vana_free(void* ptr)
vana_free:
    push ebp
    mov ebp, esp
    mov eax, 5 ; Command free (Frees the allocated memory for this process)
    push dword[ebp+8] ; Variable "ptr"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void vana_process_load_start(const char* filename)
vana_process_load_start:
    push ebp
    mov ebp, esp
    mov eax, 6 ; Command process load start
    push dword[ebp+8] ; Variable "filename"
    int 0x80
    add esp, 4
    pop ebp
    ret

; int vana_system(struct command_argument* arguments)
vana_system:
    push ebp
    mov ebp, esp
    mov eax, 7 ; Command 7 process_system ( runs a system command based on the arguments)
    push dword[ebp+8] ; Variable "arguments"
    int 0x80
    add esp, 4
    pop ebp
    ret


; void vana_process_get_arguments(struct process_arguments* arguments)
vana_process_get_arguments:
    push ebp
    mov ebp, esp
    mov eax, 8 ; Command 8 Gets the process arguments
    push dword[ebp+8] ; Variable arguments
    int 0x80
    add esp, 4
    pop ebp
    ret

; void vana_exit()
vana_exit:
    push ebp
    mov ebp, esp
    mov eax, 9 ; Command 9 process exit
    int 0x80
    pop ebp
    ret
