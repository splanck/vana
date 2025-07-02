#ifndef VANA_H
#define VANA_H
#include <stddef.h>
#include <stdbool.h>

struct command_argument
{
    char argument[512];
    struct command_argument* next;
};

struct process_arguments
{
    int argc;
    char** argv;
};

void print(const char* filename);
int vana_getkey();

void* vana_malloc(size_t size);
void vana_free(void* ptr);
void vana_putchar(char c);
int vana_getkeyblock();
void vana_terminal_readline(char* out, int max, bool output_while_typing);
void vana_process_load_start(const char* filename);
struct command_argument* vana_parse_command(const char* command, int max);
void vana_process_get_arguments(struct process_arguments* arguments);
int vana_system(struct command_argument* arguments);
int vana_system_run(const char* command);
void vana_exit();

#endif
