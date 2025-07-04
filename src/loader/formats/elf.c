#include "elf.h"

/*
 * elf.c - Basic utilities for working with ELF headers.
 *
 * These helpers expose simple accessors for the entry address of an
 * executable.  Higher level loaders use them when preparing a program
 * for execution.
 */

void* elf_get_entry_ptr(struct elf_header* elf_header)
{
    return (void*) elf_header->e_entry;
}

uint32_t elf_get_entry(struct elf_header* elf_header)
{
    return elf_header->e_entry;
}
