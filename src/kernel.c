/*
 * kernel.c - Kernel entry point and initialization routines.
 *
 * The kernel starts executing here after the bootloader transfers
 * control from assembly stub code. All essential subsystems are
 * initialized before the first user task is launched.
 */
#include "kernel.h"
#include "string/string.h"
#include "memory/memory.h"
#include "config.h"
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
struct gdt gdt_real[VANA_TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured[VANA_TOTAL_GDT_SEGMENTS] = {
    {.base = 0x00, .limit = 0x00, .type = 0x00},                // Null segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x9a},         // Kernel code
    {.base = 0x00, .limit = 0xffffffff, .type = 0x92},         // Kernel data
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf8},         // User code
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf2},         // User data
    {.base = (uint32_t)&tss, .limit = sizeof(tss), .type = 0xE9} // TSS
};

/* Convert an ASCII character and colour into a VGA text entry */
uint16_t terminal_make_char(char c, char colour)
{
    return (colour << 8) | c;
}

/* Write a character to the screen at the given column and row */
void terminal_putchar(int x, int y, char c, char colour)
{
    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, colour);
}

/* Handle backspace by moving cursor left and clearing the cell */
void terminal_backspace()
{
    if (terminal_row == 0 && terminal_col == 0)
    {
        return;
    }

    if (terminal_col == 0)
    {
        terminal_row -= 1;
        terminal_col = VGA_WIDTH;
    }

    terminal_col -= 1;
    terminal_writechar(' ', 15);
    terminal_col -= 1;
}

/* Output a character handling special cases like newline and backspace */
void terminal_writechar(char c, char colour)
{
    if (c == '\n')
    {
        terminal_row += 1;
        terminal_col = 0;
        return;
    }

    if (c == 0x08)
    {
        terminal_backspace();
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
/* Clear the VGA text buffer and reset cursor position */
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



/* Convenience wrapper to output a null terminated string */
void print(const char* str)
{
    size_t len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        terminal_writechar(str[i], 15);
    }
}

/* Display a message and halt the system */
void panic(const char* msg)
{
    print(msg);
    while (1) {}
}

/* Activate the kernel paging directory */
void kernel_page()
{
    kernel_registers();
    paging_switch(kernel_chunk);
}

/* Load the test program twice and attach simple arguments */
void inject_process_args()
{
    struct process* process = 0;
    int res = process_load_switch("0:/blank.elf", &process);
    if (res != VANA_ALL_OK)
    {
        panic("Failed to load blank.elf\n");
    }

    struct command_argument argument;
    strcpy(argument.argument, "Testing!");
    argument.next = 0x00;
    process_inject_arguments(process, &argument);

    res = process_load_switch("0:/blank.elf", &process);
    if (res != VANA_ALL_OK)
    {
        panic("Failed to load blank.elf\n");
    }

    strcpy(argument.argument, "Abc!");
    argument.next = 0x00;
    process_inject_arguments(process, &argument);
}

/*
 * Primary kernel entry invoked after low level assembly bootstrap.
 * Initializes all core subsystems and launches the initial user task.
 */
void kernel_main()
{
    terminal_initialize();
    print("Terminal ready.\n");

    // Ensure no interrupts fire during early setup
    disable_interrupts();

    memset(gdt_real, 0x00, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, VANA_TOTAL_GDT_SEGMENTS);
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


    // Load and execute the shell as the very first user task
    struct process* process = 0;
    int res = process_load_switch("0:/shell.elf", &process);
    if (res != VANA_ALL_OK)
    {
        panic("Failed to load shell.elf\n");
    }

    // Unmask timer (IRQ0) and keyboard (IRQ1) lines now that handlers exist
    outb(0x21, 0xFC);   // enable IRQ0 and IRQ1 only
    outb(0xA1, 0xFF);   // keep all slave PIC IRQs masked

    // Enable interrupts right before jumping to the first task
    enable_interrupts();

    // The call below should never return as it switches to the first task
    task_run_first_ever_task();

    // Code below would normally run after the first task starts, but the task
    // switch should not return. Leaving it here for reference only.

    // disable_interrupts();
    // for (;;) {
    //     asm volatile("hlt");
    // }
}
