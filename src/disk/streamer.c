/*
 * Disk Streamer
 *
 * Implements a simple byte oriented interface over the sector based disk
 * driver. Each stream tracks a current byte position and reads are buffered
 * through a temporary 512 byte sector. When a request crosses a sector
 * boundary the remaining bytes are read recursively from the next sector
 * while the internal position is updated. Higher level code such as the FAT16
 * driver can therefore ignore sector boundaries and treat the disk like a
 * contiguous byte array.
 */

#include "streamer.h"
#include "memory/heap/kheap.h"
#include "config.h"

#include <stdbool.h>

// Allocate a new stream for the given disk. The starting position is zero so
// reads begin at the start of the disk.
struct disk_stream* diskstreamer_new(int disk_id)
{
    struct disk* disk = disk_get(disk_id);
    if (!disk)
    {
        return 0;
    }

    struct disk_stream* streamer = kzalloc(sizeof(struct disk_stream));
    streamer->pos = 0;
    streamer->disk = disk;
    return streamer;
}

// Set the absolute byte position within the disk stream. The next read will
// operate from this offset.
int diskstreamer_seek(struct disk_stream* stream, int pos)
{
    stream->pos = pos;
    return 0;
}

/*
 * Read `total` bytes from the current stream position into `out`. The function
 * buffers a whole 512 byte sector and copies the requested portion. If the
 * read would cross a sector boundary the remaining bytes are fetched by
 * recursively calling itself and the stream's `pos` is advanced to reflect all
 * bytes returned.
 */
int diskstreamer_read(struct disk_stream* stream, void* out, int total)
{
    int sector = stream->pos / VANA_SECTOR_SIZE;
    int offset = stream->pos % VANA_SECTOR_SIZE;
    int total_to_read = total;
    bool overflow = (offset + total_to_read) >= VANA_SECTOR_SIZE;
    char buf[VANA_SECTOR_SIZE];

    if (overflow)
    {
        total_to_read -= (offset + total_to_read) - VANA_SECTOR_SIZE;
    }

    int res = disk_read_block(stream->disk, sector, 1, buf);
    if (res < 0)
    {
        goto out;
    }

    for (int i = 0; i < total_to_read; i++)
    {
        *(char*)out++ = buf[offset + i];
    }

    // Adjust the stream
    stream->pos += total_to_read;
    if (overflow)
    {
        res = diskstreamer_read(stream, out, total - total_to_read);
    }
out:
    return res;
}

// Release the stream and its tracking information.
void diskstreamer_close(struct disk_stream* stream)
{
    kfree(stream);
}

