#include "kernel.h"
#include "string/string.h"
#include "memory/memory.h"
#include "gdt/gdt.h"
#include "task/tss.h"
#include "idt/idt.h"
#include "memory/heap/kheap.h"
#include "keyboard/keyboard.h"
#include "memory/paging/paging.h"
#include "task/process.h"
#include "task/task.h"
#include "isr80h/isr80h.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "fs/file.h"
#include "status.h"
#include "io/io.h"
#include <stddef.h>
#include <stdint.h>

static struct paging_4gb_chunk* kernel_chunk = 0;

uint16_t* video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;

struct tss tss;
struct gdt gdt_real[4];
struct gdt_structured gdt_structured[4] = {
    {.base = 0x00, .limit = 0x00, .type = 0x00},                // Null segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x9a},         // Kernel code
    {.base = 0x00, .limit = 0xffffffff, .type = 0x92},         // Kernel data
    {.base = (uint32_t)&tss, .limit = sizeof(tss), .type = 0xE9} // TSS
};

uint16_t terminal_make_char(char c, char colour)
{
    return (colour << 8) | c;
}

void terminal_putchar(int x, int y, char c, char colour)
{
    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, colour);
}

void terminal_writechar(char c, char colour)
{
    if (c == '\n')
    {
        terminal_row += 1;
        terminal_col = 0;
        return;
    }
    
    terminal_putchar(terminal_col, terminal_row, c, colour);
    terminal_col += 1;
    if (terminal_col >= VGA_WIDTH)
    {
        terminal_col = 0;
        terminal_row += 1;
    }
}
void terminal_initialize()
{
    video_mem = (uint16_t*)(0xB8000);
    terminal_row = 0;
    terminal_col = 0;
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, y, ' ', 0);
        }
    }   
}



void print(const char* str)
{
    size_t len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        terminal_writechar(str[i], 15);
    }
}

void panic(const char* msg)
{
    print(msg);
    while (1) {}
}

void kernel_page()
{
    kernel_registers();
    paging_switch(kernel_chunk);
}

void kernel_main()
{
    terminal_initialize();
    print("Terminal ready.\n");

    // Ensure no interrupts fire during early setup
    disable_interrupts();

    memset(gdt_real, 0x00, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, 4);
    struct gdt_descriptor desc;
    desc.size = sizeof(gdt_real) - 1;
    desc.address = (uint32_t)gdt_real;
    gdt_load(&desc);
    print("GDT loaded.\n");

    // Initialize the kernel heap before paging
    kheap_init();
    print("Heap initialized.\n");

    memset(&tss, 0x00, sizeof(tss));
    tss.esp0 = 0x600000;
    tss.ss0 = GDT_KERNEL_DATA_SELECTOR;
    tss_load(GDT_TSS_SELECTOR);
    print("TSS loaded.\n");

    // With the GDT and TSS active we can setup the IDT
    idt_init();
    isr80h_register_commands();
    print("IDT initialized.\n");

    // Ignore spurious timer interrupts until proper handlers exist
    idt_register_interrupt_callback(0x20, interrupt_ignore);

    fs_init();
    disk_search_and_init();
    struct disk_stream* default_stream = diskstreamer_new(0);
    if (default_stream)
        diskstreamer_close(default_stream);
    print("Disk initialized.\n");

    keyboard_init();
    print("Keyboard initialized.\n");
    kernel_chunk =
        paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT |
                       PAGING_ACCESS_FROM_ALL);
    paging_switch(kernel_chunk);
    enable_paging();
    print("Paging enabled.\n");

    struct process* process = NULL;
    int res = process_load_switch("0:/blank.elf", &process);
    if (res != VANA_ALL_OK)
    {
        panic("Failed to load blank.elf\n");
    }

    // Unmask timer (IRQ0) and keyboard (IRQ1) lines now that handlers exist
    outb(0x21, 0xFC);   // enable IRQ0 and IRQ1 only
    outb(0xA1, 0xFF);   // keep all slave PIC IRQs masked

    enable_interrupts();
    print("Interrupts on.\n");

    task_run_first_ever_task();

    disable_interrupts();
    for (;;) {
        asm volatile("hlt");
    }
}
