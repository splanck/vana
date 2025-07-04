# Documentation

This directory serves as the central collection of notes and design guides for
the Vana kernel.  Each document focuses on a specific subsystem and describes
the major structures, algorithms and conventions used in the implementation.
Reading through these files is the best way to gain context on how the project
is organized and why certain design decisions were made.

The subsystems described here combine to form a minimal yet complete operating
system.  The bootloader prepares the environment and loads the kernel, which in
turn sets up memory management, interrupt handling and hardware drivers.  On top
of these foundations are the scheduler and system call layer that allow user
programs to run.  Together, these components provide a clear picture of how the
kernel pieces fit together.

Below is a table of all available topics.

| Document | Description |
| --- | --- |
| [bootloader.md](bootloader.md) | Bootloader Overview |
| [gdt.md](gdt.md) | Global Descriptor Table |
| [idt.md](idt.md) | Interrupt Descriptor Table |
| [memory_management.md](memory_management.md) | Memory Management |
| [disk_driver.md](disk_driver.md) | Disk Driver and Streamer |
| [keyboard_driver.md](keyboard_driver.md) | Keyboard Driver Overview |
| [syscalls.md](syscalls.md) | System Calls |
| [fat16.md](fat16.md) | FAT16 Filesystem Overview |
| [scheduler.md](scheduler.md) | Task Scheduler |
| [elf_loading.md](elf_loading.md) | ELF Loader and Paging Setup |
| [files.md](files.md) | Source File Overview |

For a high level introduction see the [project README](../README.md).
