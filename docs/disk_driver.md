# Disk Driver and Streamer

This document explains how the kernel communicates with its single IDE hard
disk.  The ATA driver in `src/disk/disk.c` uses classic programmed I/O (PIO)
registers to issue commands and read sector data.  All higher level accesses
ultimately pass through this low level routine.

During early boot `disk_search_and_init()` probes the drive.  It zeroes the
global `disk` structure, assigns the standard 512 byte sector size and marks
the device as `VANA_DISK_TYPE_REAL`.  The function then calls
`fs_resolve()` so the FAT16 filesystem driver can populate the
`filesystem` and `fs_private` fields.  Once initialised the rest of the
kernel obtains the disk via `disk_get()` and performs reads through the
filesystem layer.

To make file I/O easier, a small streaming layer lives in
`streamer.c`.  A `disk_stream` keeps track of a byte offset and provides
helpers to seek and read.  The streamer internally issues sector reads and
copies the requested bytes so callers see a contiguous stream of data.

## Disk Structure

`disk.h` defines a simple `struct disk` describing a physical drive.  Only one
real disk (`VANA_DISK_TYPE_REAL`) is currently supported.  The structure
contains:

- `type` – identifies whether the disk is real or emulated.
- `sector_size` – normally `VANA_SECTOR_SIZE` (512 bytes).
- `id` – the bus identifier used when issuing commands.
- `filesystem` – pointer to the resolved filesystem driver.
- `fs_private` – storage for driver-specific data such as FAT16 details.

During boot `disk_search_and_init()` in `disk.c` clears the global `disk`
instance, fills in the sector size and type and then calls `fs_resolve()` to
detect a filesystem.  In practice this resolves to the FAT16 driver which
initialises private structures for reading the partition.  The
function `disk_get()` simply returns a pointer to this structure when index 0 is
requested so the rest of the kernel can open files through the filesystem layer.

## LBA Reads

Actual sector IO happens in `disk_read_sector()`. This routine performs ATA
PIO reads using Logical Block Addressing (LBA):

1. Write the drive select and highest LBA bits to port `0x1F6`.
2. Send the sector count to `0x1F2`.
3. Output the low, mid and high bytes of the LBA to `0x1F3`, `0x1F4` and
   `0x1F5`.
4. Issue the `0x20` *read sectors* command by writing to `0x1F7`.
5. For each sector poll `0x1F7` until the DRQ bit is set, then read 256
   words from `0x1F0` into the caller's buffer.

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
requested portion to the caller. When the request would cross a sector boundary
the function first copies only the remaining bytes from the current sector and
then recurses to fetch the next one.  The caller simply receives the full byte
range while the streamer updates its internal `pos` field to track progress.

This small streaming layer simplifies filesystem code such as the FAT16 driver
which uses it to read directory entries and file data without dealing with
sector boundaries itself.

