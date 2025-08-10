#include "formats/elfloader.h"
#include "fs/file.h"
#include "status.h"
#include <stdbool.h>
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "string/string.h"

/*
 * Minimal ELF64 loader. Responsible for validating headers and
 * recording the address ranges of PT_LOAD segments so that the
 * caller can map them into memory with appropriate permissions.
 */

static const char elf_signature[] = {0x7f, 'E', 'L', 'F'};

static bool elf_valid_signature(void* buffer)
{
    return memcmp(buffer, (void*) elf_signature, sizeof(elf_signature)) == 0;
}

static bool elf_valid_class(struct elf_header* header)
{
    return header->e_ident[EI_CLASS] == ELFCLASS64;
}

static bool elf_valid_encoding(struct elf_header* header)
{
    return header->e_ident[EI_DATA] == ELFDATA2LSB;
}

static bool elf_is_executable(struct elf_header* header)
{
    return header->e_type == ET_EXEC;
}

static bool elf_has_program_header(struct elf_header* header)
{
    return header->e_phoff != 0;
}

void* elf_memory(struct elf_file* file)
{
    return file->elf_memory;
}

struct elf_header* elf_header(struct elf_file* file)
{
    return file->elf_memory;
}

struct elf64_shdr* elf_sheader(struct elf_header* header)
{
    return (struct elf64_shdr*)((uint64_t)header + header->e_shoff);
}

struct elf64_phdr* elf_pheader(struct elf_header* header)
{
    if (header->e_phoff == 0)
        return 0;
    return (struct elf64_phdr*)((uint64_t)header + header->e_phoff);
}

struct elf64_phdr* elf_program_header(struct elf_header* header, int index)
{
    return &elf_pheader(header)[index];
}

struct elf64_shdr* elf_section(struct elf_header* header, int index)
{
    return &elf_sheader(header)[index];
}

void* elf_phdr_phys_address(struct elf_file* file, struct elf64_phdr* phdr)
{
    return elf_memory(file) + phdr->p_offset;
}

void* elf_virtual_base(struct elf_file* file)
{
    return file->virtual_base_address;
}

void* elf_virtual_end(struct elf_file* file)
{
    return file->virtual_end_address;
}

void* elf_phys_base(struct elf_file* file)
{
    return file->physical_base_address;
}

void* elf_phys_end(struct elf_file* file)
{
    return file->physical_end_address;
}

static int elf_validate_loaded(struct elf_header* header)
{
    return (elf_valid_signature(header) &&
            elf_valid_class(header) &&
            elf_valid_encoding(header) &&
            elf_has_program_header(header) &&
            elf_is_executable(header)) ? VANA_ALL_OK : -EINFORMAT;
}

static int elf_process_phdr_pt_load(struct elf_file* file, struct elf64_phdr* phdr)
{
    if (file->virtual_base_address >= (void*) phdr->p_vaddr || file->virtual_base_address == 0)
    {
        file->virtual_base_address = (void*) phdr->p_vaddr;
        file->physical_base_address = elf_memory(file) + phdr->p_offset;
    }

    uint64_t end_virtual = phdr->p_vaddr + phdr->p_memsz;
    if (file->virtual_end_address <= (void*) end_virtual || file->virtual_end_address == 0)
    {
        file->virtual_end_address = (void*) end_virtual;
        file->physical_end_address = elf_memory(file) + phdr->p_offset + phdr->p_filesz;
    }

    return 0;
}

static int elf_process_pheader(struct elf_file* file, struct elf64_phdr* phdr)
{
    int res = 0;
    switch(phdr->p_type)
    {
        case PT_LOAD:
            res = elf_process_phdr_pt_load(file, phdr);
        break;
    }
    return res;
}

static int elf_process_pheaders(struct elf_file* file)
{
    int res = 0;
    struct elf_header* header = elf_header(file);
    for (int i = 0; i < header->e_phnum; i++)
    {
        struct elf64_phdr* phdr = elf_program_header(header, i);
        res = elf_process_pheader(file, phdr);
        if (res < 0)
            break;
    }
    return res;
}

static int elf_process_loaded(struct elf_file* file)
{
    int res = 0;
    struct elf_header* header = elf_header(file);
    res = elf_validate_loaded(header);
    if (res < 0)
        return res;

    return elf_process_pheaders(file);
}

struct elf_file* elf_file_new()
{
    return kzalloc(sizeof(struct elf_file));
}

void elf_file_free(struct elf_file* file)
{
    if (!file)
        return;

    if (file->elf_memory)
        kfree(file->elf_memory);

    kfree(file);
}

void elf_close(struct elf_file* file)
{
    elf_file_free(file);
}

int elf_load(const char* filename, struct elf_file** file_out)
{
    int res = 0;
    struct elf_file* elf_file = elf_file_new();
    if (!elf_file)
    {
        res = -ENOMEM;
        goto out;
    }

    strncpy(elf_file->filename, filename, sizeof(elf_file->filename));

    FILE* fd = fopen(filename, "r");
    if (!fd)
    {
        res = -EIO;
        goto out;
    }

    fseek(fd, 0, SEEK_END);
    int file_size = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    void* ptr = kzalloc(file_size);
    if (!ptr)
    {
        res = -ENOMEM;
        fclose(fd);
        goto out;
    }

    if (fread(ptr, file_size, 1, fd) != 1)
    {
        res = -EIO;
        fclose(fd);
        kfree(ptr);
        goto out;
    }

    elf_file->in_memory_size = file_size;
    elf_file->elf_memory = ptr;

    fclose(fd);

    res = elf_process_loaded(elf_file);
    if (res < 0)
    {
        elf_file_free(elf_file);
        goto out;
    }

    *file_out = elf_file;

out:
    return res;
}

void* elf_get_entry_ptr(struct elf_header* header)
{
    return (void*) header->e_entry;
}

uint64_t elf_get_entry(struct elf_header* header)
{
    return header->e_entry;
}

