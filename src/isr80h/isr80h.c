/*
 * System call registration for interrupt 0x80.
 * isr80h_register_commands() populates the global command table
 * (isr80h_commands in idt.c) with handler functions.
 * The table holds VANA_MAX_ISR80H_COMMANDS entries indexed by
 * the enumeration in isr80h.h.
 */
#include "isr80h.h"
#include "idt/idt.h"
#include "io.h"
#include "heap.h"
#include "process.h"
#include "misc.h"

void isr80h_register_commands()
{
    isr80h_register_command(ISR80H_COMMAND0_SUM, isr80h_command0_sum);
    isr80h_register_command(ISR80H_COMMAND1_PRINT, isr80h_command1_print);
    isr80h_register_command(ISR80H_COMMAND2_GETKEY, isr80h_command2_getkey);
    isr80h_register_command(ISR80H_COMMAND3_PUTCHAR, isr80h_command3_putchar);
    isr80h_register_command(ISR80H_COMMAND4_MALLOC, isr80h_command4_malloc);
    isr80h_register_command(ISR80H_COMMAND5_FREE, isr80h_command5_free);
    isr80h_register_command(ISR80H_COMMAND6_PROCESS_LOAD_START, isr80h_command6_process_load_start);
    isr80h_register_command(ISR80H_COMMAND7_INVOKE_SYSTEM_COMMAND, isr80h_command7_invoke_system_command);
    isr80h_register_command(ISR80H_COMMAND8_GET_PROGRAM_ARGUMENTS, isr80h_command8_get_program_arguments);
    isr80h_register_command(ISR80H_COMMAND9_EXIT, isr80h_command9_exit);
}
