# Interrupt Descriptor Table (IDT)

The IDT is the CPU data structure that routes every interrupt and exception to
its handler.  Each entry is eight bytes long, containing the handler's address
split into two 16‑bit fields plus type and privilege bits.  When the processor
raises an interrupt it uses the index in the table to locate the descriptor and
jumps to the specified handler code.

At boot the kernel remaps the Programmable Interrupt Controller so hardware IRQs
start at vector `0x20`, leaving vectors `0x00`–`0x1F` for CPU exceptions.  Some
IRQs can fire spuriously; IRQ7, IRQ14 and IRQ15 are treated this way and their
handlers merely send an end‑of‑interrupt without further processing.

User programs issue system calls via vector `0x80`.  The descriptor for this
vector points to `isr80h_wrapper`, which builds a minimal `interrupt_frame` and
invokes the C dispatcher.  The handler runs the requested command in kernel mode
and places the return value in `eax` before returning to user space.

This kernel uses the x86 Interrupt Descriptor Table (IDT) to dispatch both
hardware interrupts and software requests.  The IDT setup is split across three
files:

* `src/idt/idt.asm` – low level assembly stubs and helper routines.
* `src/idt/idt.c` – C helpers that populate the IDT and dispatch callbacks.
* `src/idt/idt.h` – data structures and public APIs used by the rest of the code.

## Assembly helpers (`idt.asm`)

The assembly file exposes several global symbols.  `enable_interrupts` and
`disable_interrupts` simply execute the `sti` and `cli` instructions.  `idt_load`
invokes `lidt` with a pointer to an `idtr_desc` structure.  A macro named
`interrupt` expands to 256 small handler stubs that save registers, push the
current stack pointer and interrupt number, and then call the common
`interrupt_handler` in C.  A jump table labelled `interrupt_pointer_table`
contains pointers to all these stubs so the C code can easily install them into
the IDT.  Finally, `isr80h_wrapper` builds a minimal `interrupt_frame`, calls the
C routine `isr80h_handler` and returns the value produced by that routine.  This
wrapper is used for the `0x80` system‑call vector.

## C implementation (`idt.c`)

`idt.c` maintains an array of 256 `idt_desc` entries and an `idtr_desc` used by
`idt_load`.  During `idt_init` every descriptor is initialised from
`interrupt_pointer_table` with the kernel gate attributes.  The special vector
`0x80` is associated with `isr80h_wrapper` so that user programs can invoke
system calls.  After loading the table, early exceptions (vectors 0–31) and
the spurious IRQs 7, 14 and 15 (vectors `0x27`, `0x2E` and `0x2F`) register a
default callback named `interrupt_ignore` that simply acknowledges the
interrupt.

When an interrupt fires the assembly stub ends up in `interrupt_handler`.
This routine looks up a function pointer in the `interrupt_callbacks` array and
executes it when available.  Unknown interrupts cause a panic after printing the
interrupt number.  The handler also sends an end‑of‑interrupt command to the PIC
for hardware IRQs.

The file also implements a lightweight system‑call dispatcher.  Kernel services
can register functions with `isr80h_register_command`, indexed by a numeric
command id.  `isr80h_handler` switches to the kernel page tables,
saves the task state and then executes the registered command.  Whatever value
it returns is placed back in `eax` by `isr80h_wrapper` before returning to the
caller.

## Public definitions (`idt.h`)

`idt.h` defines the `idt_desc` and `idtr_desc` structures required by the
hardware as well as the `interrupt_frame` layout pushed by the assembly
wrappers.  This structure mirrors the registers saved by the stubs so C
callbacks can inspect user state.  The header also exposes helper functions
such as `enable_interrupts`,
`disable_interrupts`, `isr80h_handler` and `idt_register_interrupt_callback` so
other subsystems (for example the keyboard driver) can hook their own handlers.

Together these pieces provide a small but complete interrupt layer with a simple
callback mechanism and a user‑mode system‑call gateway on vector `0x80`.
