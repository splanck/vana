# Porting Plan: Expand Vana Kernel Using Example Sources

This document outlines a step‑by‑step plan to evolve `vana` from its minimal
kernel into a feature rich system similar to the reference found under
`example_kernel_src/PeachOS-master/`.  Each step references the directories and
files in the example sources that should be consulted or copied when adding the
feature.

## 1. Core Utility Functions
- **Goal**: Provide basic C runtime utilities.
- **Example**: `example_kernel_src/PeachOS-master/src/memory/memory.c` and
  `src/string/string.c`.
- **Tasks**:
  1. Create `src/memory/` with `memory.c` and `memory.h` implementing `memset`,
     `memcpy`, and `memcmp`.
  2. Create `src/string/` with `string.c` and `string.h` for routines such as
     `strlen`, `strcpy`, `strncpy`, `strnlen`, etc.
  3. Add `src/io/` with `io.asm` and `io.h` to provide `insb`, `insw`, `outb`,
     `outw` for port I/O.
  4. Update the `Makefile` to compile these new sources.

## 2. Global Descriptor Table and Task State Segment
- **Goal**: Enter protected mode cleanly and provide a task switch segment.
- **Example**: `example_kernel_src/PeachOS-master/src/gdt/` and
  `src/task/tss.asm`.
- **Tasks**:
  1. Implement GDT structures and conversion routines (`gdt.c`, `gdt.h`).
  2. Provide assembly helpers in `gdt.asm` to load the table.
  3. Define a basic `tss` structure and an assembly routine in `tss.asm` to
     load it with `ltr`.
  4. Extend `kernel_main()` so it builds the GDT entries, loads the table, and
     loads the TSS before enabling paging.

## 3. Interrupt Descriptor Table and Handlers
- **Goal**: Allow hardware interrupts and exceptions.
- **Example**: `example_kernel_src/PeachOS-master/src/idt/`.
- **Tasks**:
  1. Add `src/idt/idt.c`, `idt.h`, and `idt.asm` implementing the interrupt
     descriptor table, default handlers, and wrappers.
  2. Provide functions to register interrupt callbacks and to enable/disable
     interrupts (`enable_interrupts`, `disable_interrupts`).
  3. Call `idt_init()` from `kernel_main()` after the GDT is loaded.

## 4. Paging and Kernel Heap
- **Goal**: Create paging structures and a dynamic memory allocator.
- **Example**: `src/memory/paging/` and `src/memory/heap/` from the reference
  kernel.
- **Tasks**:
  1. Implement page directory/table management (`paging.c`, `paging.h`,
     `paging.asm`).
  2. Provide a simple heap allocator using fixed-size blocks
     (`heap.c`, `heap.h`) and a kernel wrapper (`kheap.c`, `kheap.h`).
  3. Reserve heap space in the linker script if necessary and initialise the
     heap early in `kernel_main()`.
  4. Create an initial 4‑GB paging chunk, switch to it, and enable paging.

## 5. Basic Device Drivers
- **Goal**: Support keyboard input and disk access.
- **Example**: `src/keyboard/` and `src/disk/` in the example tree.
- **Tasks**:
  1. Port the PS/2 keyboard driver (`keyboard.c`, `classic.c`, `keyboard.h`) and
     hook its interrupt (IRQ1) through the IDT.
  2. Add disk driver and sector streamer (`disk.c`, `streamer.c`, headers).
  3. Initialise these drivers in `kernel_main()` once interrupts are working.

## 6. File System Layer (FAT16)
- **Goal**: Read files from disk using a simple filesystem.
- **Example**: `src/fs/` and `src/fs/fat/` in the reference.
- **Tasks**:
  1. Implement a filesystem abstraction (`file.c`, `file.h`) with descriptors,
     open/read/seek/stat/close operations.
  2. Port the path parser (`pparser.c`, `pparser.h`) and the FAT16 driver
     (`fat16.c`, `fat16.h`).
  3. Add filesystem initialisation during kernel startup and mount the boot
     disk.

## 7. Task and Process Management
- **Goal**: Create multitasking and the ability to load ELF programs.
- **Example**: `src/task/` and `src/loader/formats/` in the reference kernel.
- **Tasks**:
  1. Port the task scheduler (`task.c`, `task.asm`, `task.h`) and process
     support (`process.c`, `process.h`).
  2. Implement ELF loading helpers (`elf.c`, `elfloader.c`) under
     `src/loader/formats/`.
  3. Extend `kernel_main()` to load a user program from the filesystem and
     start the first task via `task_run_first_ever_task()`.

## 8. System Call Interface (INT 0x80)
- **Goal**: Allow user programs to interact with the kernel.
- **Example**: `src/isr80h/` in PeachOS.
- **Tasks**:
  1. Add an interrupt handler for vector 0x80 and a table of syscall commands.
  2. Implement initial commands: printing, keyboard input, `malloc`/`free`, and
     process creation.
  3. Provide small user‑space stubs (similar to `programs/stdlib/`) that wrap the
     interrupt with convenient C functions.

## 9. Shell and User Programs
- **Goal**: Boot into a simple shell loaded from disk.
- **Example**: `programs/` in the example source.
- **Tasks**:
  1. Port the simple shell program and the example `blank` test program.
  2. Adjust the Makefile to build these user programs into ELF binaries and copy
     them onto the disk image.
  3. Ensure the kernel can launch the shell after initialising all subsystems.

## 10. Future Enhancements
- Once the above baseline is working, additional features from the reference
  kernel—such as more system calls, improved drivers, and better scheduling—can
  be ported incrementally.

This plan progressively introduces subsystems in an order that mirrors their
initialisation sequence in `example_kernel_src/PeachOS-master/src/kernel.c`.
Following these steps will transform the tiny `vana` kernel into a much more
capable operating system.
