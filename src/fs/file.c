/*
 * Generic file API implementation.
 *
 * Filesystems register with fs_insert_filesystem() and supply callbacks for
 * opening, reading and closing files. When a file is opened a new descriptor
 * is allocated from file_descriptors[] and used to dispatch all subsequent
 * operations to the owning filesystem.
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

struct filesystem* filesystems[VANA_MAX_FILESYSTEMS];
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

// Register a filesystem implementation with the kernel
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

// Insert any filesystems that are built in to the kernel
static void fs_static_load()
{
    fs_insert_filesystem(fat16_init());
}

// Initialize the filesystems array and load built-in filesystems
static void fs_load()
{
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

// Prepare file descriptor tables and load filesystems
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

// Determine which registered filesystem owns the provided disk
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

// Open a file and return a new descriptor index
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

// Populate stat with information about an open descriptor
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

// Close a previously opened descriptor
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

// Seek to a new position in an open descriptor
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

// Read data from an open descriptor
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

