#ifndef ELFLOADER_H
#define ELFLOADER_H

#include <stdint.h>
#include <stddef.h>

#include "elf.h"
#include "config.h"

struct elf_file
{
    char filename[VANA_MAX_PATH];

    int in_memory_size;

    /**
     * The physical memory address that this elf file is loaded at
     */
    void* elf_memory;

    /**
     * The virtual base address of this binary
     */
    void* virtual_base_address;

    /**
     * The ending virtual address
     */
    void* virtual_end_address;

    /**
     * The physical base address of this binary
     */
    void* physical_base_address;

    /**
     * The physical end address of this bunary
     */
    void* physical_end_address;

};

int elf_load(const char* filename, struct elf_file** file_out);
struct elf_file* elf_file_new();
void elf_file_free(struct elf_file* file);

void elf_close(struct elf_file* file);
/* Starting virtual address of the loaded ELF image. */
void* elf_virtual_base(struct elf_file* file);
/* Virtual address where the ELF image ends. */
void* elf_virtual_end(struct elf_file* file);
/* Starting physical address of the loaded ELF image. */
void* elf_phys_base(struct elf_file* file);
/* Final physical address used by the ELF image. */
void* elf_phys_end(struct elf_file* file);

/* Get a pointer to the ELF header within the file. */
struct elf_header* elf_header(struct elf_file* file);
/* Return the first section header entry. */
struct elf32_shdr* elf_sheader(struct elf_header* header);
/* Pointer to the raw ELF data in memory. */
void* elf_memory(struct elf_file* file);
/* Return the first program header entry. */
struct elf32_phdr* elf_pheader(struct elf_header* header);
/* Retrieve the program header at the given index. */
struct elf32_phdr* elf_program_header(struct elf_header* header, int index);
/* Retrieve the section header at the given index. */
struct elf32_shdr* elf_section(struct elf_header* header, int index);
/* Convert a program header into a physical address. */
void* elf_phdr_phys_address(struct elf_file* file, struct elf32_phdr* phdr);

#endif
