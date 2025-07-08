#ifndef KERNEL_H
#define KERNEL_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 20


/** Entry point after the bootloader hands control to the kernel. */
void kernel_main();
/** Write a string directly to the VGA text console. */
void print(const char* str);
/** Display a panic message and halt. */
void panic(const char* msg);
/** Switch to the kernel's page directory. */
void kernel_page();
/** Restore segment registers for kernel mode. */
void kernel_registers();
/** Low level helper to output a single character with colour. */
void terminal_writechar(char c, char colour);

#define ERROR(value)  ((void*)(value))
#define ERROR_I(value) ((int)(value))
#define ISERR(value) ((int)(value) < 0)

#endif
