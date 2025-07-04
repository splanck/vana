# FAT16 Filesystem Overview

This document summarizes how the FAT16 driver and helper modules work together to provide file access. Key files include `src/fs/fat/fat16.c/h`, the generic file interface in `src/fs/file.c/h` and the path parser in `src/fs/pparser.c/h`.

## Path Parsing

`pathparser.c` converts strings like `0:/folder/file.txt` into a linked list of parts. `pathparser_parse()` validates the `drive:/` prefix, allocates a `struct path_root` and then repeatedly calls `pathparser_parse_path_part()` to split the rest of the path. Each part is stored in a `struct path_part` where `part` points to the string and `next` links to the following component.

`pathparser_free()` walks this list freeing every part. The parsed structure lets higher level code traverse directories without repeatedly tokenising strings.

## File Descriptor Layer

`file.h` defines a generic `struct filesystem` with callbacks for `open`, `read`, `seek`, `stat` and `close`. `file.c` keeps arrays of registered filesystems and active `file_descriptor` objects. Each descriptor stores its numeric index, a pointer to the filesystem, a private pointer supplied by the driver and the disk it operates on.

`fs_init()` clears these tables and inserts the FAT16 driver via `fat16_init()`. `fopen()` uses the path parser to obtain the drive number and path parts, resolves the disk with `disk_get()` and delegates the rest to the filesystem's `open` callback. When successful a descriptor is allocated and returned. `fread()`, `fseek()`, `fstat()` and `fclose()` simply look up the descriptor and call the corresponding driver functions.

## Directory Traversal in FAT16

The FAT16 driver reads the root directory using `fat16_get_root_directory()` which calculates its sector location from the header fields. Entries are loaded into a `struct fat_directory` array; `total`, `sector_pos` and `ending_sector_pos` record where the directory resides on disk.

`fat16_find_item_in_directory()` iterates through a `fat_directory` comparing names created by `fat16_get_full_relative_filename()`. If a match is a subdirectory, `fat16_load_fat_directory()` reads that directory's cluster into memory. `fat16_get_directory_entry()` walks the linked path parts produced by the parser, repeatedly searching child directories until the target item is located. Each node in this traversal is represented by a `struct fat_item` which may point to either a `fat_directory` or a single `fat_directory_item` depending on whether it is a directory or a file.

## Reading Clusters

Files are stored as chains of clusters. `fat16_cluster_to_sector()` converts a cluster number to an absolute sector after the root directory. The FAT chain is followed with `fat16_get_fat_entry()` which uses `diskstreamer_seek()` and `diskstreamer_read()` on the `fat_read_stream` field of `struct fat_private`.

`fat16_get_cluster_for_offset()` determines which cluster holds a given file offset by walking these entries. Actual data is fetched in `fat16_read_internal_from_stream()`: it computes the byte position of the cluster, seeks the `cluster_read_stream` and reads up to a whole cluster. If more bytes are required the function recurses, seamlessly handling files that span multiple clusters.

`fat16_read()` wraps this helper to implement the `read` callback used by `fread()`. The companion functions `fat16_seek()`, `fat16_stat()` and `fat16_close()` manipulate a `struct fat_file_descriptor` which stores the current offset and a pointer to the `fat_item` representing the file. Each call updates this structure so subsequent operations continue from the correct location.

Together these pieces allow the kernel to parse paths, traverse directories and read file contents from a FAT16 formatted disk.

## Example usage
```c
int fd = fopen("0:/README.TXT", "r");
char buf[128];
int bytes = fread(buf, 1, sizeof(buf), fd);
fclose(fd);
```
`fopen()` resolves the path using the parser, `fread()` follows the FAT chain for the file and fills `buf`; `fclose()` releases the descriptor when finished.
