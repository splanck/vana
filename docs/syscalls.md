# System Calls (`isr80h`)

This kernel exposes a small system call interface on interrupt `0x80`. User
programs trigger the interrupt with a command number in `eax` and arguments on
the stack. The dispatcher is implemented in `src/idt/idt.c` and individual
commands live under `src/isr80h/`.

## Command registration (`isr80h.c`)

`enum ISR80H_COMMANDS` in `isr80h.h` assigns numeric ids to each command. During
kernel start‑up `isr80h_register_commands()` registers handlers for every id by
calling `isr80h_register_command`. The function resides in `src/isr80h/isr80h.c`
and simply ties each id to one of the handlers described below.

## Available commands

The handlers below receive a pointer to `interrupt_frame` and operate on the
currently running task. They all return a value via `eax`.

### `isr80h_command0_sum` (`misc.c`)
Adds the first two integers from the user stack and returns the result.

### `isr80h_command1_print` (`io.c`)
Copies a user string into a kernel buffer and prints it to the terminal.

### `isr80h_command2_getkey` (`io.c`)
Returns the next character from the keyboard queue.

### `isr80h_command3_putchar` (`io.c`)
Writes a single character to the terminal.

### `isr80h_command4_malloc` (`heap.c`)
Allocates memory from the calling process heap. The size is taken from the user
stack and the returned pointer is user‑space accessible.

### `isr80h_command5_free` (`heap.c`)
Frees a pointer previously allocated with command 4.

### `isr80h_command6_process_load_start` (`process.c`)
Loads and starts a user program specified by path. Control switches to the new
process after loading.

### `isr80h_command7_invoke_system_command` (`process.c`)
Runs a program with arguments supplied as a `command_argument` array. The helper
injects the arguments into the new process before switching tasks.

### `isr80h_command8_get_program_arguments` (`process.c`)
Copies the current process argument vector into a user provided structure.

### `isr80h_command9_exit` (`process.c`)
Terminates the current process and schedules the next task.

