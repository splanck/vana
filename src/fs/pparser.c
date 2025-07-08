/*
 * Very small path parser used by the FAT16 driver.
 *
 * Paths must be absolute and follow the ``<drive>:/dir/file`` pattern.
 * ``pathparser_parse()`` breaks the string into a ``struct path_root`` which
 * contains the drive number and a linked list of ``path_part`` components. The
 * filesystem then iterates over this list to walk directories one part at a
 * time.
 *
 * Only digits 0-9 are accepted for the drive number and no handling for ".."
 * or redundant slashes is provided.  This matches the current FAT16
 * implementation which assumes a single drive and simple 8.3 style names.
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
 * Free a parsed path structure returned by ``pathparser_parse``.
 *
 * @param root  The root of the path list to free. Passing NULL is safe.
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
 * Parse an absolute path into its components.
 *
 * @param path                   NUL terminated path string ``<drive>:/...``.
 * @param current_directory_path Unused placeholder for future relative paths.
 * @return                       Newly allocated ``struct path_root`` or NULL on
 *                               failure. Use ``pathparser_free`` when done.
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

