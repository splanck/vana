/*
 * Generic file API implementation.
 *
 * The kernel maintains two global tables:
 *  - ``filesystems`` stores pointers to registered ``struct filesystem``
 *    objects. Drivers call ``fs_insert_filesystem()`` during
 *    initialization so that the VFS layer can delegate path operations to
 *    them.
 *  - ``file_descriptors`` tracks open files. ``fopen()`` allocates a new
 *    entry when a path is successfully resolved and uses the index as the
 *    public file descriptor returned to callers.
 *
 * Only a FAT16 driver is currently provided and the implementation assumes
 * 512 byte sectors and classic 8.3 filenames.  Long filename extensions and
 * other FAT variants (such as FAT32) are not supported.
 */
#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "string/string.h"
#include "disk/disk.h"
#include "fat/fat16.h"
#include "status.h"
#include "kernel.h"

// Registered filesystems. A NULL entry means the slot is free.
struct filesystem* filesystems[VANA_MAX_FILESYSTEMS];

// Active file descriptors indexed by ``fd-1``. Descriptor 0 is unused so that
// a valid descriptor is always non-zero from the caller's perspective.
struct file_descriptor* file_descriptors[VANA_MAX_FILE_DESCRIPTORS];

// Locate a free entry in the filesystem table
static struct filesystem** fs_get_free_filesystem()
{
    for (int i = 0; i < VANA_MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] == 0)
        {
            return &filesystems[i];
        }
    }

    return 0;
}

/*
 * Register a filesystem implementation with the kernel.
 *
 * @param filesystem  Pointer to the driver specific struct filesystem.
 *                    The pointer is stored in the global table.
 */
void fs_insert_filesystem(struct filesystem* filesystem)
{
    struct filesystem** fs = fs_get_free_filesystem();
    if (!fs)
    {
        print("Problem inserting filesystem");
        while (1) {}
    }

    *fs = filesystem;
}

/*
 * Insert any filesystem drivers that are compiled directly into the kernel.
 *
 * At present this function registers only the FAT16 driver which means all
 * path operations ultimately resolve to a FAT16 formatted disk.
 */
static void fs_static_load()
{
    fs_insert_filesystem(fat16_init());
}

/*
 * Reset the filesystem registry and add any statically compiled drivers.
 */
static void fs_load()
{
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

/*
 * Initialise the filesystem layer.
 *
 * Clears the descriptor table and registers any built-in filesystem drivers.
 * This must be called once during kernel startup before any file operations.
 */
void fs_init()
{
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

// Free a file descriptor that was previously allocated
static void file_free_descriptor(struct file_descriptor* desc)
{
    file_descriptors[desc->index-1] = 0;
    kfree(desc);
}

// Allocate a new file descriptor structure for an opened file
static int file_new_descriptor(struct file_descriptor** desc_out)
{
    int res = -ENOMEM;
    for (int i = 0; i < VANA_MAX_FILE_DESCRIPTORS; i++)
    {
        if (file_descriptors[i] == 0)
        {
            struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor));
            desc->index = i + 1; // descriptors start at 1
            file_descriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }

    return res;
}

// Resolve a descriptor index to its file_descriptor structure
static struct file_descriptor* file_get_descriptor(int fd)
{
    if (fd <= 0 || fd >= VANA_MAX_FILE_DESCRIPTORS)
    {
        return 0;
    }

    int index = fd - 1; // descriptors start at 1
    return file_descriptors[index];
}

/*
 * Determine which registered filesystem can handle the given disk.
 *
 * @param disk  Physical disk structure returned by the disk layer.
 * @return      Pointer to a matching filesystem driver or NULL if none
 *              recognise the disk.
 */
struct filesystem* fs_resolve(struct disk* disk)
{
    for (int i = 0; i < VANA_MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] && filesystems[i]->resolve(disk) == 0)
        {
            return filesystems[i];
        }
    }

    return 0;
}

// Convert an fopen() mode string ("r", "w", etc.) to a FILE_MODE value
static FILE_MODE file_get_mode_by_string(const char* str)
{
    FILE_MODE mode = FILE_MODE_INVALID;
    if (strncmp(str, "r", 1) == 0)
    {
        mode = FILE_MODE_READ;
    }
    else if (strncmp(str, "w", 1) == 0)
    {
        mode = FILE_MODE_WRITE;
    }
    else if (strncmp(str, "a", 1) == 0)
    {
        mode = FILE_MODE_APPEND;
    }

    return mode;
}

/*
 * Open a file by path.
 *
 * @param filename  Absolute path in the form "<drive>:/dir/file".
 * @param mode_str  Standard C style mode string ("r", "w", "a").
 * @return          A positive descriptor index on success, or 0 on error.
 */
int fopen(const char* filename, const char* mode_str)
{
    int res = 0;
    struct disk* disk = NULL;
    FILE_MODE mode = FILE_MODE_INVALID;
    void* descriptor_private_data = NULL;
    struct file_descriptor* desc = 0;

    struct path_root* root_path = pathparser_parse(filename, NULL);
    if (!root_path)
    {
        res = -EINVARG;
        goto out;
    }

    if (!root_path->first)
    {
        res = -EINVARG;
        goto out;
    }

    disk = disk_get(root_path->drive_no);
    if (!disk)
    {
        res = -EIO;
        goto out;
    }

    if (!disk->filesystem)
    {
        res = -EIO;
        goto out;
    }

    mode = file_get_mode_by_string(mode_str);
    if (mode == FILE_MODE_INVALID)
    {
        res = -EINVARG;
        goto out;
    }

    descriptor_private_data = disk->filesystem->open(disk, root_path->first, mode);
    if (ISERR(descriptor_private_data))
    {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    res = file_new_descriptor(&desc);
    if (res < 0)
    {
        goto out;
    }

    desc->filesystem = disk->filesystem;
    desc->private = descriptor_private_data;
    desc->disk = disk;
    res = desc->index;

out:
    if (res < 0)
    {
        if (root_path)
        {
            pathparser_free(root_path);
        }

        if (disk && descriptor_private_data)
        {
            disk->filesystem->close(descriptor_private_data);
        }

        if (desc)
        {
            file_free_descriptor(desc);
        }

        res = 0;
    }

    return res;
}

/*
 * Retrieve file statistics for an open descriptor.
 *
 * @param fd    Descriptor returned by ``fopen``.
 * @param stat  Output structure filled with size and flag information.
 * @return      ``VANA_ALL_OK`` on success or a negative error code.
 */
int fstat(int fd, struct file_stat* stat)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc)
    {
        return -EIO;
    }

    res = desc->filesystem->stat(desc->disk, desc->private, stat);
    return res;
}

/*
 * Close a previously opened descriptor.
 *
 * @param fd  Descriptor obtained from ``fopen``.
 * @return    ``VANA_ALL_OK`` on success or a negative error code.
 */
int fclose(int fd)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc)
    {
        return -EIO;
    }

    res = desc->filesystem->close(desc->private);
    if (res == VANA_ALL_OK)
    {
        file_free_descriptor(desc);
    }

    return res;
}

/*
 * Seek to a new position within an open file.
 *
 * @param fd     Descriptor returned by ``fopen``.
 * @param offset Byte offset relative to ``whence``.
 * @param whence One of ``SEEK_SET``, ``SEEK_CUR`` or ``SEEK_END``.
 * @return       ``VANA_ALL_OK`` on success or a negative error code.
 */
int fseek(int fd, int offset, FILE_SEEK_MODE whence)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc)
    {
        return -EIO;
    }

    res = desc->filesystem->seek(desc->private, offset, whence);
    return res;
}

/*
 * Read data from an open descriptor.
 *
 * @param ptr   Buffer to store the data.
 * @param size  Size of each object to read in bytes.
 * @param nmemb Number of objects to read.
 * @param fd    Descriptor obtained from ``fopen``.
 * @return      Number of bytes read or a negative error code.
 */
int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd)
{
    if (size == 0 || nmemb == 0 || fd < 1)
    {
        return -EINVARG;
    }

    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc)
    {
        return -EINVARG;
    }

    return desc->filesystem->read(desc->disk, desc->private, size, nmemb, (char*)ptr);
}

