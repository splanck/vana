/*
 * ATA disk driver using Programmed I/O (PIO) operations.
 *
 * The primary IDE bus exposes a set of well known ports:
 *   0x1F0 – data register used to read 16-bit words
 *   0x1F2 – sector count
 *   0x1F3 – LBA low byte
 *   0x1F4 – LBA mid byte
 *   0x1F5 – LBA high byte
 *   0x1F6 – drive/head register
 *   0x1F7 – command/status register
 *
 * Only a single drive is supported. Higher layers interact with this
 * driver via the filesystem which ultimately calls `disk_read_block()`.
 */
#include "disk.h"
#include "io/io.h"
#include "config.h"
#include "status.h"
#include "memory/memory.h"

static struct disk disk;

/*
 * Read one or more sectors from the primary ATA drive.
 *
 * @param lba   Logical block address of the first sector.
 * @param total Number of sectors to read.
 * @param buf   Destination buffer (must hold total * 512 bytes).
 * @return      Zero on success, negative error code otherwise.
 */
static int disk_read_sector(int lba, int total, void* buf)
{
    /* Select the drive and output the 28-bit LBA and sector count. */
    outb(0x1F6, (lba >> 24) | 0xE0);      // drive/head register
    outb(0x1F2, total);                   // number of sectors to read
    outb(0x1F3, (unsigned char)(lba & 0xff));  // LBA low
    outb(0x1F4, (unsigned char)(lba >> 8));    // LBA mid
    outb(0x1F5, (unsigned char)(lba >> 16));   // LBA high
    outb(0x1F7, 0x20);                  // send READ SECTORS command

    unsigned short* ptr = (unsigned short*) buf;
    for (int b = 0; b < total; b++)
    {
        /* Wait for the drive to assert the Data Request (DRQ) bit. */
        char c = insb(0x1F7);
        while(!(c & 0x08))
        {
            c = insb(0x1F7);
        }

        /* Read one sector (256 words) from the data port. */
        for (int i = 0; i < 256; i++)
        {
            *ptr = insw(0x1F0);
            ptr++;
        }
    }

    return 0;
}

/*
 * Probe for the primary disk and initialise the global descriptor.
 * `fs_resolve()` is invoked to attach a filesystem driver so that later
 * calls through the VFS can transparently access this device.
 */
void disk_search_and_init()
{
    memset(&disk, 0, sizeof(disk));
    disk.type = VANA_DISK_TYPE_REAL;
    disk.sector_size = VANA_SECTOR_SIZE;
    disk.id = 0;
    disk.filesystem = fs_resolve(&disk);
}

struct disk* disk_get(int index)
{
    if (index != 0)
        return 0;

    return &disk;
}

/*
 * Public wrapper used by the filesystem layer (e.g. FAT16) to read
 * one or more sectors. The disk pointer is validated before the
 * request is forwarded to `disk_read_sector()`.
 */
int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf)
{
    if (idisk != &disk)
    {
        return -EIO;
    }

    return disk_read_sector(lba, total, buf);
}

