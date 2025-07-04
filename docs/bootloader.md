# Bootloader Overview

The bootloader is the first code executed once the BIOS hands off control. It
is built from `src/boot/boot.asm` and the `ORG 0x7C00` directive ensures the
sector runs from the well‑known `0x7C00` address where the BIOS loads it. The
entire image fits in one 512 byte sector and ends with the `0xAA55` signature so
the BIOS recognizes it as a valid FAT16 boot sector.

Its main job is to transition the processor into 32‑bit protected mode and load
the kernel. A tiny Global Descriptor Table is defined in the boot code and its
size and address are described by the `gdt_descriptor` structure. After the
descriptor is loaded with `lgdt`, the bootloader sets the PE bit in `CR0` and
performs a far jump so execution continues in 32‑bit mode.

Once the CPU is operating with 32‑bit instructions, the bootloader reads sector
**1** from disk for **100** consecutive sectors and stores the data at address
`0x0100000`. It then jumps to this location to begin executing the kernel
properly.

## FAT16 Boot Sector

The first instructions jump over the boot parameter block. Immediately
following them is a standard FAT16 header defining the OEM string, disk layout
and extended BPB fields. The boot code ends with the `0xAA55` signature so the
BIOS recognizes it as a valid boot sector.

Key fields include:

- `BytesPerSector` set to 0x200 (512 bytes)
- `SectorsPerCluster` set to 0x80
- `ReservedSectors`, `FATCopies`, `RootDirEntries` and other values required by
  the FAT16 format

## GDT Setup and Protected Mode Switch

After basic register setup in real mode, the bootloader loads its Global
Descriptor Table (GDT). The GDT defines three descriptors:

1. A null descriptor
2. A code segment descriptor (`gdt_code`) used once in protected mode
3. A data segment descriptor (`gdt_data`) for all data segments

The table size and address are described by `gdt_descriptor`, which sits right
after `gdt_end`. The bootloader loads this descriptor with `lgdt`, sets the PE
bit in `CR0` and performs a far jump to `load32`, entering 32‑bit mode.

## Protected Mode Initialization

The `load32` label executes with 32‑bit instructions. It reloads all segment
registers with the data segment selector and enables the A20 line through port
`0x92`. Once the CPU is in protected mode, the bootloader prepares to read the
kernel by placing the target LBA, sector count, and destination address in
`EAX`, `ECX` and `EDI` respectively and calling `ata_lba_read`. In the source
code this means loading sector **1** from disk for **100** consecutive sectors
into address `0x0100000` before jumping there.

## ATA LBA Load Logic

The `ata_lba_read` routine communicates with the primary ATA controller to load
sectors from disk:

1. The highest LBA bits are written to port `0x1F6` with the drive select bits.
2. The sector count goes to port `0x1F2`.
3. The low, mid and high bytes of the LBA are written to ports `0x1F3`, `0x1F4`
   and `0x1F5`.
4. Sending `0x20` to port `0x1F7` issues the read command.
5. For each sector, the code polls `0x1F7` until the DRQ bit is set, then reads
   256 words from port `0x1F0` into memory using `rep insw`.

After the requested sectors are loaded into memory, execution jumps to the
kernel entry point at `0x0100000`.
