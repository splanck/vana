# Global Descriptor Table (GDT)

Segmentation on x86 is described by descriptors stored in the Global
Descriptor Table. Each descriptor defines a region of memory and the
privilege level it operates under. The kernel installs its own GDT
early in boot so that both kernel and user code run with predictable
limits and access rights.

The table used by Vana contains six entries: a null descriptor, kernel
code and data descriptors, user code and data descriptors, and a Task
State Segment (TSS) descriptor. Convenience constants defined in
`src/config.h` expose their selectors to the rest of the kernel,
including `KERNEL_CODE_SELECTOR` (0x08), `KERNEL_DATA_SELECTOR` (0x10),
`USER_CODE_SEGMENT` (0x1b) and `USER_DATA_SEGMENT` (0x23). The TSS entry
is selected with `GDT_TSS_SELECTOR`.

With these selectors loaded the CPU executes kernel code in ring&nbsp;0
while user programs run in ring&nbsp;3. The TSS descriptor enables interrupts
and system calls to switch to the kernel stack, forming the bridge
between user and kernel mode.

This kernel uses the x86 Global Descriptor Table to define the memory
segments that code and data run in.  The implementation is split between
`gdt.c`, `gdt.h` and the small assembly helper `gdt.asm`.

### Descriptor creation

Descriptors are first declared in the friendlier `struct gdt_structured` form.
`gdt_structured_to_gdt()` encodes them into the packed eight-byte layout via
`encode_gdt_entry` before `gdt_load()` installs the table.

## Data structures

`gdt.h` declares three structures:

* `struct gdt` – Layout of an encoded descriptor exactly as the CPU
  expects it.  Each entry is eight bytes long and packed without any
  padding.  Fields map directly to the bit layout of a real GDT
  descriptor.
* `struct gdt_structured` – A friendlier representation used in C code.
  It stores a 32‑bit base address, a 32‑bit limit and a type byte.
* `struct gdt_descriptor` – Used with the `lgdt` instruction.  It holds
  the size of the GDT in bytes minus one and the 32‑bit memory address of
  the table.

The selectors used throughout the kernel are also defined here:
`GDT_KERNEL_CODE_SELECTOR` (0x08) and `GDT_KERNEL_DATA_SELECTOR` (0x10)
correspond to the first two kernel segments, while `GDT_TSS_SELECTOR`
(0x28) is reserved for the TSS descriptor.

`src/config.h` defines convenience aliases `KERNEL_CODE_SELECTOR` and
`KERNEL_DATA_SELECTOR` which simply map to the GDT values above so other
subsystems can include a single header when referencing segment selectors.

## Encoding descriptors

`gdt_structured_to_gdt()` in `gdt.c` converts an array of
`gdt_structured` entries to their binary form.  Each entry is processed
by the internal function `encode_gdt_entry()` which fills the byte array
representing a descriptor.

The function first checks whether the requested limit fits the rules of
the x86 GDT.  Limits larger than 64 KiB must be 4 KiB aligned.  When a
limit exceeds 64 KiB the granularity flag is set and the limit is shifted
right by 12 bits to express it in 4 KiB pages.  The relevant bits of the
limit are split between the low two bytes and the high nibble of the
sixth byte of the descriptor.

The base address is written into bytes 2–4 and the final byte, producing
the 32‑bit base expected by the CPU.  The descriptor type (access byte)
occupies byte 5 and the granularity or size flags occupy byte 6.

## Loading the GDT

The actual loading of the table is handled in `gdt.asm`.  The exported

### Descriptor creation

Descriptors are first declared in the friendlier `struct gdt_structured` form.
`gdt_structured_to_gdt()` encodes them into the packed eight-byte layout via
`encode_gdt_entry` before `gdt_load()` installs the table.
function `gdt_load(struct gdt_descriptor *desc)` performs the following
steps:

1. Load the descriptor with the `lgdt` instruction.
2. Reload all segment registers with the kernel data selector so they
   reference the new table.
3. Perform a far jump to reload the code segment (`CS`) with the kernel
   code selector.  Execution continues at label `flush` after the jump.

After these steps the processor uses the newly created GDT for segment
translation.

