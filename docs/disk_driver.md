# Disk Driver and Streamer

This document summarizes how the kernel reads data from the disk using the
low level ATA driver in `src/disk/disk.c`/`disk.h` and the convenience
stream interface implemented by `streamer.c`/`streamer.h`.

## Disk Structure

`disk.h` defines a simple `struct disk` describing a physical drive. Only one
real disk (`VANA_DISK_TYPE_REAL`) is supported. The structure stores the
sector size (normally `VANA_SECTOR_SIZE` which is 512 bytes), an ID used when
communicating with the controller and pointers to the filesystem implementation
and any private data it requires.

During boot `disk_search_and_init()` in `disk.c` initialises this global
structure and attempts to resolve the filesystem with `fs_resolve()`. The
function `disk_get()` simply returns a pointer to this structure when index 0 is
requested.

## LBA Reads

Actual sector IO happens in `disk_read_sector()`. This routine performs ATA
PIO reads using Logical Block Addressing (LBA):

1. The highest LBA bits combined with the drive select are written to port
   `0x1F6`.
2. The number of sectors to read goes to `0x1F2`.
3. The low, mid and high bytes of the LBA are sent to `0x1F3`, `0x1F4` and
   `0x1F5`.
4. Writing `0x20` to `0x1F7` issues the read command.
5. For each sector the code polls port `0x1F7` until the DRQ bit is set and then
   reads 256 words from port `0x1F0` into the caller's buffer.

`disk_read_block()` is a thin wrapper used by the rest of the kernel to read one
or more sectors starting at a given LBA.

## Buffered Stream Interface

Higher level code typically does not operate directly on sectors. The file
`streamer.c` provides a byte oriented `disk_stream` structure with the
following helpers:

- `diskstreamer_new()` – allocate a stream for the given disk ID.
- `diskstreamer_seek()` – move the current position in bytes.
- `diskstreamer_read()` – read an arbitrary number of bytes.
- `diskstreamer_close()` – release the stream.

`diskstreamer_read()` calculates the LBA sector and offset for the current
position. It reads the sector into a temporary 512 byte buffer and copies the
requested portion to the caller. If the read spans multiple sectors the
function recursively reads the next sector so callers see a contiguous byte
stream. After each call the internal `pos` member is updated so subsequent reads
continue from the new location.

This small streaming layer simplifies filesystem code such as the FAT16 driver
which uses it to read directory entries and file data without dealing with
sector boundaries itself.

