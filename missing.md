# Missing Features in Vana Kernel

After comparing `vana` with the reference kernel in `example_kernel_src/PeachOS-master`, several pieces of functionality are absent in the `vana` source tree.

## 1. System Call `SYSTEM_COMMAND0_SUM`
- **Files missing:** `src/isr80h/misc.c` and `src/isr80h/misc.h` exist only in the example kernel. They define a system call that returns the sum of two integers passed on the current task's stack.
- **Enumeration gap:** In `src/isr80h/isr80h.h`, commands start with `ISR80H_COMMAND1_PRINT`, whereas the example includes `SYSTEM_COMMAND0_SUM` before print. This missing entry means Vana lacks the first system call implemented in the example.
- **Makefile entry:** The example Makefile compiles `./build/isr80h/misc.o` but the Vana Makefile has no rule for it.

## 2. Terminal Backspace Handling
- The reference kernel's `kernel.c` provides `terminal_backspace()` and checks for the backspace character (0x08) in `terminal_writechar()` to erase characters on the text screen.
- Vana's `kernel.c` does not include this function or the backspace check, so backspace input is not handled at the terminal level.

## 3. Sample Program Loading with Arguments
- In the example kernel, `kernel_main()` loads `blank.elf` twice and injects command-line arguments using `process_inject_arguments()` before starting the first task.
- Vana's `kernel_main()` simply loads `shell.elf` and does not demonstrate argument injection.

These differences mean the Vana kernel lacks the sum system call, terminal backspace support, and the demonstration code for loading programs with arguments present in the example kernel. All other directories and files closely mirror the reference tree, aside from naming differences (e.g., `PEACHOS` vs `VANA` prefixes) and an extra `pic` directory in Vana which is not present in the example.
