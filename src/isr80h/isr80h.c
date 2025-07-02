#include "isr80h.h"
#include "idt/idt.h"
#include "io.h"
#include "heap.h"
#include "process.h"

void isr80h_register_commands()
{
    isr80h_register_command(ISR80H_COMMAND1_PRINT, isr80h_command1_print);
    isr80h_register_command(ISR80H_COMMAND2_GETKEY, isr80h_command2_getkey);
    isr80h_register_command(ISR80H_COMMAND3_PUTCHAR, isr80h_command3_putchar);
    isr80h_register_command(ISR80H_COMMAND4_MALLOC, isr80h_command4_malloc);
    isr80h_register_command(ISR80H_COMMAND5_FREE, isr80h_command5_free);
    isr80h_register_command(ISR80H_COMMAND6_PROCESS_LOAD_START, isr80h_command6_process_load_start);
}
