#ifndef KERNEL_H
#define KERNEL_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 20

#include "memory/paging/paging.h"

extern struct paging_4gb_chunk* kernel_chunk;

void kernel_main();
void print(const char* str);
void panic(const char* msg);
void kernel_page();
void kernel_registers();
void terminal_writechar(char c, char colour);

#define ERROR(value)  ((void*)(value))
#define ERROR_I(value) ((int)(value))
#define ISERR(value) ((int)(value) < 0)

#endif
