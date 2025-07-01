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

static void fs_static_load()
{
    fs_insert_filesystem(fat16_init());
}

static void fs_load()
{
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

void fs_init()
{
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

static void file_free_descriptor(struct file_descriptor* desc)
{
    file_descriptors[desc->index-1] = 0;
    kfree(desc);
}

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

static struct file_descriptor* file_get_descriptor(int fd)
{
    if (fd <= 0 || fd >= VANA_MAX_FILE_DESCRIPTORS)
    {
        return 0;
    }

    int index = fd - 1; // descriptors start at 1
    return file_descriptors[index];
}

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

