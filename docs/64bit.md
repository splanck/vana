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
   - Link the kernel with `linker64.ld` using `x86_64-elf-gcc` and `x86_64-elf-ld` (see the example Makefile in `64bitexample.tar.gz`).
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

## File‑by‑File Migration Guide

The checklist below summarises concrete edits required across the source tree. It
is intentionally verbose so that each component can be migrated and verified in
isolation.  Follow the order roughly from low level boot code up to userland.

### Assembly Files to Rewrite
The following assembly sources require complete 64‑bit rewrites. Each has an
equivalent implementation in `64bitexample.tar.gz` that can be referenced when
performing the conversion.

* **src/boot/boot.asm** – rebuild with `nasm -f elf64` and perform the long
  mode transition using 64‑bit registers.
* **src/kernel.asm** – convert the entry stub to use `rax`, `rbx` and other
  64‑bit registers before jumping to `kernel_main`.
* **src/io/io.asm** – update port I/O helpers to use `rax`/`rdx` and the
  64‑bit instruction forms.
* **src/memory/paging/paging.asm** – replace 32‑bit paging code with 4‑level
  page table operations and 64‑bit `cr3` loads.
* **src/task/task.asm** – save and restore `r8–r15` and follow the System V
  ABI for context switches.
* **src/task/tss.asm** – implement the long‑mode TSS layout and load it with
  `ltr`.
* **src/gdt/gdt.asm** – emit 64‑bit descriptors compatible with long mode and
  load them via `lgdt`.
* **src/idt/idt.asm** – create 16‑byte IDT gates and set IST indices for each
  handler.

### Boot Code

* **src/boot/boot.asm** – remove the 32‑bit real mode loader. Create
  `src/boot64/boot.asm` based on the example. It must:
  - Enable A20, build page tables and switch to long mode.
  - Load `kernel64.elf` at 0x100000 and jump to its 64‑bit entry label.
* **src/kernel.asm** – rewrite as a 64‑bit stub called from the bootloader. Set
  up segment registers and the stack then call `kernel_main`.

:::start-task{title="Convert boot code"}
- Stub out the existing 32‑bit assembly in `src/boot/boot.asm`.
- Add `boot64/boot.asm` and update `Makefile` rules to assemble with
  `nasm -f elf64`.
- Provide a 64‑bit version of `kernel.asm` that matches the new entry flow.
:::

### Descriptor Tables

* **src/gdt/gdt.c** and **src/gdt/gdt.h** – extend descriptor structures to hold
  64‑bit base addresses. Update `gdt_structured_to_gdt` and helper macros.
* **src/gdt/gdt.asm** – emit long‑mode compatible descriptor entries and load
  the table with `lgdt`.
* **src/task/tss.asm** and **src/task/tss.h** – define a 64‑bit TSS with
  separate stacks for interrupts.
* **src/idt/idt.c** and **src/idt/idt.asm** – implement 16‑byte IDT gates and
  configure the IST fields for critical handlers.

:::start-task{title="Upgrade descriptor tables"}
- Modify GDT structures to use `uint64_t` bases and limits.
- Generate 64‑bit IDT descriptors in `idt.asm` and adapt `idt_load`.
- Add a long‑mode TSS definition and load it in `gdt.c` during
  initialization.
:::

### Paging and Memory

* **src/memory/paging/paging.*** – replace the 32‑bit directory logic with
  routines for PML4, PDPT, PD and PT creation.
* **src/memory/heap/kheap.c** – adjust pointer types to `uintptr_t` and confirm
  allocations work above the 4GiB boundary.
* **src/memory/memory.c** – review `memset`, `memcpy` and `memcmp` for
  size‑t/ptr differences when compiled in 64‑bit mode.

:::start-task{title="Implement new paging"}
- Add `paging64.c`/`paging64.h` and migrate kernel setup in `kernel.c` to use
  these helpers.
- Ensure the higher half mapping is established before enabling paging.
- Update heap initialisation to allocate from the new virtual addresses.
:::

### Task Switching and Syscalls

* **src/task/task.asm** – rewrite push/pop logic for the System V ABI. Include
  registers `r8–r15` and expand the trap frame structure.
* **src/task/task.c** and **src/task/process.c** – store 64‑bit `rip` and
  `rsp` values in the `struct task` and `struct process` records.
* **src/isr80h/isr80h.c** – change the dispatcher to read parameters from the
  `rdi`, `rsi`, `rdx`, `rcx`, `r8` and `r9` registers.

:::start-task{title="Refactor context switch"}
- Expand register save areas in `task.h` to include 64‑bit registers.
- Update assembly stubs in `task.asm` for the new stack frame layout.
- Implement `syscall`/`sysret` entry points and adapt userland wrappers.
:::

### Kernel Core

* **src/config.h** – update selectors, virtual addresses and stack sizes for
  64‑bit. Define `KERNEL_VMA` (e.g. `0xFFFFFFFF80000000`).
* **src/kernel.c** – allocate the new paging structures and set up the GDT/IDT
  before entering the scheduler.
* **src/io/io.asm** and other port I/O helpers – confirm all instructions use
  the 64‑bit register forms.

:::start-task{title="Update kernel core"}
- Search all `.c` files for `uint32_t` and convert pointers to `uint64_t` or
  `uintptr_t`.
- Adjust the linker script to place the kernel at `KERNEL_VMA`.
- Ensure all assembly files specify `[BITS 64]` and are assembled as ELF64.
:::

### C Source Updates by Directory
Refer to the modules inside `64bitexample.tar.gz` for working 64‑bit code.

* **src/gdt** – update descriptors and loader routines for 64‑bit bases.
* **src/idt** – create 16‑byte gate structures and fill 64‑bit handler addresses.
* **src/memory/paging** – provide PML4/PDP table creation helpers and rewrite paging initialization.
* **src/task** – store 64‑bit register state and update scheduler logic.
* **src/isr80h** – fetch syscall parameters from `rdi` onward following the System V ABI.
* **src/loader** – extend the ELF loader to parse 64‑bit headers and program segments.
* **src/kernel.c** – build 64‑bit page tables, load the new GDT/IDT and start the first task.
* **src/config.h** – define 64‑bit addresses, segment selectors and stack sizes.

### Userland and Libraries

* **example_libc** and **programs/** – rebuild with `x86_64-elf-gcc`. Update
  inline assembly in the libc to use the new syscall conventions.

:::start-task{title="Finalize userland"}
- Replace any 32‑bit specific assembly in `example_libc` with 64‑bit
  equivalents.
- Recompile the shell and test utilities and run them under QEMU.
:::

