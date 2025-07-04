# Source File Overview

This document acts as a guide to the kernel's source tree.  It lists every
C and assembly file under `src/` so newcomers can quickly locate the code
responsible for a particular subsystem or feature.

Having a short explanation next to each filename makes navigating the project
easier when exploring the implementation or adding new functionality.

The list below was verified against `find src -name '*.c' -or -name '*.asm'`
and currently covers all 33 source files.

## Files

- `src/boot/boot.asm` - 16‑bit boot sector that switches the CPU into protected mode and loads the kernel from disk. It also sets up a minimal GDT before jumping to the kernel entry point.
- `src/kernel.asm` - Early assembly routine that sets up segment registers, remaps the PIC and jumps to `kernel_main`. Provides `kernel_registers` helper for restoring segment registers after task switches.
- `src/kernel.c` - C entry point of the kernel. Initializes core subsystems such as the GDT, IDT, paging, heap, disk driver and keyboard, then loads the first user program and starts the task scheduler.
- `src/io/io.asm` - Provides simple port I/O helper functions (`insb`, `insw`, `outb`, `outw`) used throughout the kernel for hardware access.
- `src/pic/pic.c` - Contains routines to interact with the Programmable Interrupt Controller including sending end-of-interrupt signals.
- `src/string/string.c` - Implements basic C string and utility functions like `strlen`, `strncpy`, `int_to_string` and character tests.
- `src/gdt/gdt.asm` - Assembly helper for loading the Global Descriptor Table at runtime via `gdt_load`.
- `src/gdt/gdt.c` - Builds GDT descriptors in C and converts structured entries to the binary format expected by the CPU.
- `src/idt/idt.asm` - Assembly support code for setting up the IDT. Defines generic interrupt stubs, an ISR80h wrapper and helpers to enable/disable interrupts.
- `src/idt/idt.c` - Initializes the IDT table in C, registers interrupt handlers and provides a syscall dispatch mechanism via interrupt `0x80`.
- `src/memory/memory.c` - Basic memory manipulation routines (`memset`, `memcmp`, `memcpy`) used throughout the codebase.
- `src/memory/heap/heap.c` - Generic block‑based heap allocator. Manages free/used blocks and provides malloc/free primitives for arbitrary heaps.
- `src/memory/heap/kheap.c` - Kernel heap implementation built on top of `heap.c`. Initializes the kernel heap region and exposes `kmalloc`, `kzalloc` and `kfree` helpers.
- `src/memory/paging/paging.asm` - Low level functions for loading a page directory and enabling paging on the CPU.
- `src/memory/paging/paging.c` - High level paging utilities. Creates 4GB paging chunks, maps/unmaps memory and translates virtual addresses.
- `src/disk/disk.c` - ATA disk driver used to read sectors from the disk. Detects a single disk and exposes functions to read blocks.
- `src/disk/streamer.c` - Implements a convenience streaming interface over the disk driver allowing random access reads using a file‑like position pointer.
- `src/keyboard/keyboard.c` - Keyboard manager that tracks registered keyboard drivers, maintains a key buffer and exposes functions to retrieve keystrokes.
- `src/keyboard/classic.c` - Implements a PS/2 keyboard driver using the classic scancode set. Handles shift and capslock state and converts scancodes to ASCII.
- `src/fs/file.c` - Generic file API handling open/close/read/seek operations. Manages file descriptors and delegates to filesystem drivers.
- `src/fs/pparser.c` - Path parsing helper that splits strings like `0:/dir/file` into drive numbers and path components.
- `src/fs/fat/fat16.c` - FAT16 filesystem driver. Parses FAT structures, resolves paths, reads directory entries and files, and exposes the `fat16` `struct filesystem` implementation.
- `src/loader/formats/elf.c` - Small helpers for working with ELF headers such as fetching the entry address from an executable.
- `src/loader/formats/elfloader.c` - Loads ELF binaries into memory, validates headers and sets up paging for user processes.
- `src/task/task.asm` - Assembly routines for task switching. Restores registers, performs `iretd` to enter user mode and provides user register setup helpers.
- `src/task/tss.asm` - Loads the Task State Segment selector into the CPU with the `ltr` instruction.
- `src/task/task.c` - Core scheduler and task management code. Maintains the task list, switches contexts and copies data between tasks and kernel space.
- `src/task/process.c` - Higher level process management. Loads executables, allocates memory on behalf of processes and cleans up on exit.
- `src/isr80h/isr80h.c` - Syscall registration and dispatch for interrupt `0x80`. Maps command numbers to handler functions.
- `src/isr80h/io.c` - Syscall implementations for printing, reading keys and writing characters to the terminal.
- `src/isr80h/heap.c` - Syscalls for allocating and freeing memory inside a process's address space.
- `src/isr80h/misc.c` - Miscellaneous syscall example: simple integer sum operation used for testing.
- `src/isr80h/process.c` - Process‑related syscalls such as loading a program, invoking shell commands, retrieving arguments and exiting.
