# 64-bit Porting Plan

This document outlines a high level plan for evolving the existing 32‑bit Vana kernel into a 64‑bit implementation.  The archive `64bitexample.tar.gz` located in the repository root provides a minimal 64‑bit kernel used as a reference for the tasks below.

## Overview

Migrating to 64‑bit requires work across the entire codebase: a new toolchain, boot code that enters long mode, updated paging structures and revisions to every assembly routine and data structure.  The changes are invasive so the process is broken down into small steps.

## Start Tasks

The following tasks give a starting point for exploring the current code and the reference example.  Completing them will surface the areas that need modifications for 64‑bit support.

:::start-task{title="Review current Vana source"}
- Read the boot sequence described in `docs/bootloader.md` and inspect `src/boot/boot.asm`.
- Review descriptor tables in `src/gdt` and `src/idt`.
- Examine paging setup under `src/memory`.
- Skim the scheduler and system call code in `src/task` and `src/isr80h`.
:::

:::start-task{title="Inspect the 64-bit example"}
- Extract `64bitexample.tar.gz` and read its build scripts and linker script.
- Check how the example enters long mode in `boot.asm` and how it sets up page tables.
- Note differences in calling conventions and data types (e.g. use of `uint64_t`).
:::

## Porting Plan

1. **Cross‑Compiler**
   - Build an `x86_64-elf` GCC and Binutils toolchain similar to the existing 32‑bit one.
   - Update the `Makefile` to use the new compiler and switch assembly sources to `nasm -f elf64`.
2. **Bootloader**
   - Replace the 32‑bit boot code with a new loader that enables long mode.
   - Use the reference `boot.asm` from the example kernel as a guide.
3. **Paging**
   - Introduce 4‑level paging structures (PML4, PDPT, PD, PT).
   - Modify memory management routines to allocate and map 64‑bit pages.
4. **Descriptor Tables**
   - Expand the GDT entries to 64‑bit descriptors and set up a `TSS` valid for long mode.
   - Adjust IDT entries to the 64‑bit gate format.
5. **Context Switching and Syscalls**
   - Rework task switching assembly in `src/task/task.asm` for the 64‑bit calling convention.
   - Implement a syscall entry point using the `syscall`/`sysret` instructions.
6. **Kernel and User Code**
   - Audit all structures and pointers for 32‑bit assumptions.
   - Update constants in `src/config.h` to 64‑bit addresses.
7. **Linker Script**
   - Create a new `linker.ld` targeting the `elf64-x86-64` format.
   - Map the kernel to a higher half location similar to the example script.
8. **User Programs**
   - Recompile userland with the 64‑bit compiler and ensure ABI compatibility with the new syscall layer.

Each step should be validated in QEMU before moving to the next.  The reference kernel provides minimal implementations for most of these areas and can be consulted throughout the process.

