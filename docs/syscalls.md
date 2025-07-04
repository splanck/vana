# System Calls (`isr80h`)

The kernel exposes all user services through interrupt `0x80`, commonly known
as the **isr80h** interface.  Each request is identified by a numeric command
defined in `enum ISR80H_COMMANDS`.  The interface is intentionally small and
relies only on general registers and the stack for passing arguments.

User programs trigger a system call by placing the desired command number in
`eax`, pushing any arguments onto the stack and executing `int $0x80`.  The
assembly wrapper `isr80h_wrapper` located in `src/idt/idt.asm` saves registers,
constructs an `interrupt_frame` and hands execution to the C dispatcher.

Inside the kernel `isr80h_handler` (implemented in `src/idt/idt.c`) switches to
the kernel page tables, saves the current task state and invokes the function
associated with the command.  Once the handler returns, `isr80h_wrapper` places
its result back into `eax` before returning to user mode.

## Command registration (`isr80h.c`)

`enum ISR80H_COMMANDS` in `isr80h.h` assigns numeric ids to each command. During
boot `kernel.c` calls `isr80h_register_commands()` which registers every handler
with `isr80h_register_command`.  To add a new system call, extend the
enumeration, implement a handler matching the `ISR80H_COMMAND` prototype and
register it within `isr80h_register_commands()`.

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

