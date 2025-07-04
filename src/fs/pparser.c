/*
 * Path parser for FAT16 formatted paths.
 *
 * Paths use the form "<drive>:/dir/file". The parser splits the
 * string into a drive number and a linked list of components so
 * the FAT16 driver can traverse directories.
 */
#include "pparser.h"
#include "config.h"
#include "kernel.h"
#include "string/string.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"

/*
 * Validate that the path is in the form "<digit>:/".
 * Returns non-zero if valid.
 */
static int pathparser_path_valid_format(const char* filename)
{
    int len = strnlen(filename, VANA_MAX_PATH);
    return (len >= 3 && isdigit(filename[0]) && memcmp((void*)&filename[1], ":/", 2) == 0);
}

/*
 * Extracts the drive number from the start of the path.
 * Advances the pointer beyond "<digit>:/" and returns the drive.
 * Returns -EBADPATH if the format is invalid.
 */
static int pathparser_get_drive_by_path(const char** path)
{
    if (!pathparser_path_valid_format(*path))
    {
        return -EBADPATH;
    }

    int drive_no = tonumericdigit((*path)[0]);

    // Skip the "0:/" style drive specifier
    *path += 3;
    return drive_no;
}

/*
 * Allocate the root of the parsed path for the given drive.
 * Returns NULL on allocation failure.
 */
static struct path_root* pathparser_create_root(int drive_number)
{
    struct path_root* path_r = kzalloc(sizeof(struct path_root));
    if (!path_r)
    {
        return NULL;
    }

    path_r->drive_no = drive_number;
    path_r->first = 0;
    return path_r;
}

/*
 * Reads the next component of the path into a newly allocated string.
 * The input pointer is advanced past the component and any trailing slash.
 * Returns NULL if allocation fails or no more components exist.
 */
static const char* pathparser_get_path_part(const char** path)
{
    char* result_path_part = kzalloc(VANA_MAX_PATH);
    if (!result_path_part)
    {
        return NULL;
    }

    int i = 0;
    while (**path != '/' && **path != 0x00)
    {
        result_path_part[i] = **path;
        *path += 1;
        i++;
    }

    if (**path == '/')
    {
        *path += 1; // skip '/'
    }

    if (i == 0)
    {
        kfree(result_path_part);
        result_path_part = 0;
    }

    return result_path_part;
}

/*
 * Creates a new path_part structure for the next component.
 * Links it after last_part if supplied and advances the path pointer.
 * Returns NULL on memory errors or if there are no more parts.
 */
struct path_part* pathparser_parse_path_part(struct path_part* last_part, const char** path)
{
    const char* path_part_str = pathparser_get_path_part(path);
    if (!path_part_str)
    {
        return 0;
    }

    struct path_part* part = kzalloc(sizeof(struct path_part));
    if (!part)
    {
        kfree((void*)path_part_str);
        return 0;
    }

    part->part = path_part_str;
    part->next = 0;

    if (last_part)
    {
        last_part->next = part;
    }

    return part;
}

/*
 * Frees a path_root and all linked path_part structures.
 */
void pathparser_free(struct path_root* root)
{
    struct path_part* part = root->first;
    while (part)
    {
        struct path_part* next_part = part->next;
        kfree((void*)part->part);
        kfree(part);
        part = next_part;
    }

    kfree(root);
}

/*
 * Main entry that parses a full path string.
 * On success returns a newly allocated path_root containing all path parts.
 * current_directory_path is unused.
 */
struct path_root* pathparser_parse(const char* path, const char* current_directory_path)
{
    (void)current_directory_path;
    int res = 0;
    const char* tmp_path = path;
    struct path_root* path_root = NULL;
    struct path_part* first_part = NULL;
    struct path_part* part = NULL;

    if (strlen(path) > VANA_MAX_PATH)
    {
        res = -1;
        goto out;
    }

    res = pathparser_get_drive_by_path(&tmp_path);
    if (res < 0)
    {
        res = -1;
        goto out;
    }

    path_root = pathparser_create_root(res);
    if (!path_root)
    {
        res = -1;
        goto out;
    }

    first_part = pathparser_parse_path_part(NULL, &tmp_path);
    if (!first_part)
    {
        res = -1;
        goto out;
    }

    path_root->first = first_part;
    part = pathparser_parse_path_part(first_part, &tmp_path);
    while (part)
    {
        part = pathparser_parse_path_part(part, &tmp_path);
    }

out:
    if (res < 0)
    {
        if (path_root)
        {
            kfree(path_root);
            path_root = NULL;
        }
        if (first_part)
        {
            kfree(first_part);
        }
    }

    return path_root;
}

