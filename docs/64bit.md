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
   - Update the `build-toolchain.sh` script so `TARGET=x86_64-elf` and verify `x86_64-elf-gcc` is on the `PATH`.
   - Adjust the `Makefile` to prefer the 64‑bit compiler and switch assembly sources to `nasm -f elf64`.
2. **Bootloader**
   - Replace the 32‑bit boot code with a new loader that enables long mode.
   - Borrow the initialization sequence from the example kernel.
   - Load the kernel above 1 MiB, build temporary page tables and jump to the 64‑bit entry point.
3. **Paging**
   - Introduce 4‑level paging structures (PML4, PDPT, PD, PT).
   - Implement helpers to allocate and map 64‑bit pages with the correct flags.
   - Identity map the first few megabytes and map the kernel at a higher half address.
4. **Descriptor Tables**
   - Expand the GDT entries to 64‑bit descriptors and set up a `TSS` valid for long mode.
   - Adjust IDT entries to the 64‑bit gate format and configure interrupt stack tables.
5. **Context Switching and Syscalls**
   - Rework task switching assembly in `src/task/task.asm` for the 64‑bit calling convention.
   - Implement a syscall entry point using the `syscall`/`sysret` instructions.
   - Update user stack handling and privilege transitions.
6. **Kernel and User Code**
   - Audit all structures and pointers for 32‑bit assumptions.
   - Update constants in `src/config.h` to 64‑bit addresses and review pointer casts.
7. **Linker Script**
   - Create a new `linker.ld` targeting the `elf64-x86-64` format.
   - Map the kernel to a higher half location similar to the example script.
8. **User Programs**
   - Recompile userland with the 64‑bit compiler and ensure ABI compatibility with the new syscall layer.

Each step should be validated in QEMU before moving to the next.  The reference kernel provides minimal implementations for most of these areas and can be consulted throughout the process.

## Detailed Steps

The sections below expand the bullet list above and include concrete tasks for implementing the port.

### 1. Build the x86_64 Toolchain
The existing `build-toolchain.sh` script targets `i686-elf`. Create a parallel configuration that produces an `x86_64-elf` toolchain.

* Install the same dependencies but set `TARGET=x86_64-elf`.
* Build and install Binutils followed by GCC with C/C++ support.
* Ensure `$HOME/opt/cross/bin` contains `x86_64-elf-gcc` and related tools.

:::start-task{title="Create x86_64 cross toolchain"}
- Duplicate `build-toolchain.sh` or add options so it can produce the 64‑bit toolchain.
- Document the commands needed to compile and install Binutils and GCC for `x86_64-elf`.
- Verify the resulting compiler runs with `x86_64-elf-gcc -v`.
:::

### 2. Implement a 64-bit Bootloader
The bootloader must switch the processor from real mode all the way to long mode. Use the reference `boot.asm` as a starting point and adapt it for Vana.

* Keep a 16‑bit stage that loads the kernel and page tables to 0x100000.
* Enable the A20 line and enter protected mode just long enough to set up paging.
* Build minimal PML4/PDPT/PD/PT structures so `CR3` can be loaded.
* Set the `LME` bit in `EFER`, enable paging and jump to the 64‑bit kernel entry.

:::start-task{title="Write long mode bootloader"}
- Create `src/boot64/boot.asm` with real mode setup and the long mode transition code.
- Update the build to assemble this file with `nasm -f elf64`.
- Confirm the bootloader loads the kernel and reaches the 64‑bit entry in QEMU.
:::

### 3. Update Paging and Memory Management
Long mode requires four levels of page tables. The existing paging code only handles 32‑bit directories.

* Define new structures for PML4, PDPT, PD and PT entries.
* Implement helpers to allocate each table and set flags (present, write, execute disable).
* Map the kernel to a higher half address (e.g. `0xFFFFFFFF80000000`) while keeping an identity map for early boot.

:::start-task{title="Add 64-bit page table support"}
- Add `paging64.c` and `paging64.h` implementing PML4 creation and basic mapping APIs.
- Replace uses of the 32‑bit paging code in the kernel initialization path.
- Verify virtual addresses resolve correctly under QEMU.
:::

### 4. Revise Descriptor Tables
GDT entries remain eight bytes but long mode uses a 16‑byte IDT gate and a larger Task State Segment.

* Encode 64‑bit code and data segments and create a 64‑bit compatible TSS.
* Replace the old IDT setup with 64‑bit gates including interrupt stack table (IST) entries.

:::start-task{title="Upgrade GDT and IDT"}
- Rewrite `gdt.c`, `gdt.asm`, `idt.c` and `idt.asm` to emit 64‑bit descriptors.
- Define a TSS structure with an IST for fault handling.
- Ensure interrupts work after the switch to long mode.
:::

### 5. Context Switching and Syscalls
Task switching and system calls must follow the x86‑64 System V ABI.

* Save and restore all callee-saved registers including `r12–r15`.
* Use `syscall`/`sysret` for user to kernel transitions and provide a legacy interrupt stub for debugging.
* Update `task.c` and related assembly to push 64‑bit register state.

:::start-task{title="Implement 64-bit context switch"}
- Replace `task.asm` with a version that operates on 64‑bit stacks.
- Provide a new syscall dispatcher in `isr80h` that reads arguments from `rdi`, `rsi`, `rdx`, etc.
- Adapt the scheduler so tasks store 64‑bit instruction and stack pointers.
:::

### 6. Migrate Kernel and User Code
Many structures and constants assume 32‑bit sizes.

* Replace `uint32_t` pointers with `uint64_t` where necessary.
* Review driver code for assumptions about register width or address size.
* Update `src/config.h` with new high half addresses and stack sizes.

:::start-task{title="Audit and update core structures"}
- Search the source tree for 32‑bit types and convert them to 64‑bit friendly versions.
- Adjust printf/formatting helpers in the example libc to handle 64‑bit values.
:::

### 7. Create a 64-bit Linker Script
The kernel needs a new linker script targeting ELF64.

* Base the script on `64bitexample/link.lds` but keep the section layout from the existing kernel.
* Place the kernel at a high virtual address and ensure the physical load address matches the bootloader expectations.

:::start-task{title="Write linker.ld for ELF64"}
- Add `linker64.ld` and update the build system to use it when compiling for 64‑bit.
- Verify the produced ELF loads correctly by inspecting it with `readelf`.
:::

### 8. Rebuild User Programs
Userland must be compiled with the new toolchain and use the 64‑bit syscall ABI.

* Port the small example libc to 64‑bit, updating inline assembly and types.
* Recompile all programs with `x86_64-elf-gcc` and link against the new kernel headers.

:::start-task{title="Port userland to 64-bit"}
- Update the make rules under `programs/` to use the 64‑bit compiler.
- Modify syscall wrappers in the libc to pass arguments via `rdi`/`rsi`/`rdx`.
- Ensure the shell and test programs run in the 64‑bit environment.
:::

### 9. Testing and Debugging
Frequent testing avoids large integration problems.

* Boot every milestone in QEMU using `make run` equivalents for the 64‑bit build.
* Use the `-s -S` flags with QEMU to attach `gdb` and debug early crashes.
* Consider adding simple unit tests for paging and context switching routines.

:::start-task{title="Validate 64-bit kernel"}
- Provide QEMU command examples for booting the kernel with debugging enabled.
- Document common failure points and how to inspect page tables and register state in `gdb`.
:::

