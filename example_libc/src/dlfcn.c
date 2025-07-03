/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the dlfcn functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "dlfcn.h"
#include "memory.h"
#include "string.h"
#include "io.h"
#include "errno.h"
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>

struct dl_handle {
    void *mapping;
    size_t map_size;
    void *base;
    Elf64_Sym *symtab;
    const char *strtab;
    size_t nsyms;
    char *path;
    struct dl_handle *next;
};

static char dl_err[128];
static struct dl_handle *dl_list;

static void set_error(const char *msg);

/* Apply relocation entries to the mapped object. */
static int apply_relocs(struct dl_handle *h, Elf64_Rela *rela, size_t relasz)
{
#ifdef __x86_64__
    size_t n = relasz / sizeof(Elf64_Rela);
    for (size_t i = 0; i < n; i++) {
        Elf64_Addr *where = (Elf64_Addr *)((char *)h->base + rela[i].r_offset);
        Elf64_Xword type = ELF64_R_TYPE(rela[i].r_info);
        Elf64_Xword sym_index = ELF64_R_SYM(rela[i].r_info);
        switch (type) {
        case R_X86_64_RELATIVE:
            *where = (Elf64_Addr)((char *)h->base + rela[i].r_addend);
            break;
        case R_X86_64_64:
        case R_X86_64_GLOB_DAT:
        case R_X86_64_JUMP_SLOT:
            if (h->symtab && sym_index < h->nsyms) {
                Elf64_Sym *sym = &h->symtab[sym_index];
                *where = (Elf64_Addr)((char *)h->base + sym->st_value +
                                      rela[i].r_addend);
            }
            break;
        default:
            set_error("bad relocation");
            return -1;
        }
    }
    return 0;
#else
    (void)h;
    (void)rela;
    (void)relasz;
    set_error("unsupported architecture");
    return -1;
#endif
}

/* Simple pread replacement using lseek and read. */
static ssize_t pread_fd(int fd, void *buf, size_t count, off_t offset)
{
    if (lseek(fd, offset, SEEK_SET) < 0)
        return -1;
    return read(fd, buf, count);
}

/* Store an error message for retrieval by dlerror(). */
static void set_error(const char *msg)
{
    size_t len = strlen(msg);
    if (len >= sizeof(dl_err))
        len = sizeof(dl_err) - 1;
    memcpy(dl_err, msg, len);
    dl_err[len] = '\0';
}

/* Return the most recent dynamic loading error message. */
const char *dlerror(void)
{
    if (dl_err[0] == '\0')
        return NULL;
    return dl_err;
}

/* Map the shared object located at filename into memory and return a handle. */
void *dlopen(const char *filename, int flag)
{
    (void)flag;
    dl_err[0] = '\0';

#ifdef O_CLOEXEC
    int flags = O_RDONLY | O_CLOEXEC;
#else
    int flags = O_RDONLY;
#endif
    int fd = open(filename, flags);
    if (fd < 0) {
        set_error("open failed");
        return NULL;
    }

    Elf64_Ehdr eh;
    if (read(fd, &eh, sizeof(eh)) != sizeof(eh)) {
        set_error("read header");
        close(fd);
        return NULL;
    }
    if (eh.e_ident[EI_MAG0] != ELFMAG0 ||
        eh.e_ident[EI_MAG1] != ELFMAG1 ||
        eh.e_ident[EI_MAG2] != ELFMAG2 ||
        eh.e_ident[EI_MAG3] != ELFMAG3 ||
        eh.e_ident[EI_CLASS] != ELFCLASS64 ||
        eh.e_type != ET_DYN) {
        set_error("not a shared object");
        close(fd);
        return NULL;
    }
#ifdef __x86_64__
    if (eh.e_machine != EM_X86_64) {
        set_error("wrong architecture");
        close(fd);
        return NULL;
    }
#else
    set_error("unsupported architecture");
    close(fd);
    return NULL;
#endif

    if (lseek(fd, eh.e_phoff, SEEK_SET) < 0) {
        set_error("seek phdr");
        close(fd);
        return NULL;
    }

    Elf64_Phdr phdrs[16];
    if (eh.e_phnum > 16) {
        set_error("too many phdrs");
        close(fd);
        return NULL;
    }

    if (read(fd, phdrs, eh.e_phnum * sizeof(Elf64_Phdr)) != (ssize_t)(eh.e_phnum * sizeof(Elf64_Phdr))) {
        set_error("read phdrs");
        close(fd);
        return NULL;
    }

    Elf64_Addr min_vaddr = ~0ULL;
    Elf64_Addr max_vaddr = 0;
    Elf64_Addr dyn_vaddr = 0;
    for (int i = 0; i < eh.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            if (phdrs[i].p_vaddr < min_vaddr)
                min_vaddr = phdrs[i].p_vaddr;
            if (phdrs[i].p_vaddr + phdrs[i].p_memsz > max_vaddr)
                max_vaddr = phdrs[i].p_vaddr + phdrs[i].p_memsz;
        } else if (phdrs[i].p_type == PT_DYNAMIC) {
            dyn_vaddr = phdrs[i].p_vaddr;
        }
    }

    size_t map_size = max_vaddr - min_vaddr;
    void *map = mmap(NULL, map_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANON, -1, 0);
    if (map == MAP_FAILED) {
        set_error("mmap");
        close(fd);
        return NULL;
    }

    void *base = (char *)map - min_vaddr;
    for (int i = 0; i < eh.e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD)
            continue;
        int prot = 0;
        if (phdrs[i].p_flags & PF_R) prot |= PROT_READ;
        if (phdrs[i].p_flags & PF_W) prot |= PROT_WRITE;
        if (phdrs[i].p_flags & PF_X) prot |= PROT_EXEC;
        void *seg_addr = (char *)base + phdrs[i].p_vaddr;
        if (pread_fd(fd, seg_addr, phdrs[i].p_filesz, phdrs[i].p_offset) != (ssize_t)phdrs[i].p_filesz) {
            munmap(map, map_size);
            set_error("read segment");
            close(fd);
            return NULL;
        }
        if (phdrs[i].p_memsz > phdrs[i].p_filesz)
            memset((char *)seg_addr + phdrs[i].p_filesz, 0, phdrs[i].p_memsz - phdrs[i].p_filesz);
        mprotect(seg_addr, phdrs[i].p_memsz, prot);
    }

    struct dl_handle *h = malloc(sizeof(struct dl_handle));
    if (!h) {
        munmap(map, map_size);
        set_error("nomem");
        close(fd);
        return NULL;
    }
    h->mapping = map;
    h->map_size = map_size;
    h->base = base;
    h->symtab = NULL;
    h->strtab = NULL;
    h->nsyms = 0;
    h->path = strdup(filename);
    h->next = NULL;

    if (dyn_vaddr) {
        Elf64_Dyn *dyn = (Elf64_Dyn *)((char *)base + dyn_vaddr);
        Elf64_Rela *rela = NULL;
        size_t relasz = 0;
        size_t syment = sizeof(Elf64_Sym);
        for (; dyn->d_tag != DT_NULL; dyn++) {
            if (dyn->d_tag == DT_SYMTAB)
                h->symtab = (Elf64_Sym *)((char *)base + dyn->d_un.d_ptr);
            else if (dyn->d_tag == DT_STRTAB)
                h->strtab = (const char *)((char *)base + dyn->d_un.d_ptr);
            else if (dyn->d_tag == DT_HASH) {
                uint32_t *hash = (uint32_t *)((char *)base + dyn->d_un.d_ptr);
                h->nsyms = hash[1];
            } else if (dyn->d_tag == DT_SYMENT) {
                syment = dyn->d_un.d_val;
            } else if (dyn->d_tag == DT_RELA) {
                rela = (Elf64_Rela *)((char *)base + dyn->d_un.d_ptr);
            } else if (dyn->d_tag == DT_RELASZ) {
                relasz = dyn->d_un.d_val;
            }
        }
        if (h->nsyms == 0 && h->symtab && h->strtab && syment)
            h->nsyms = ((char *)h->strtab - (char *)h->symtab) / syment;
        if (rela && relasz) {
            if (apply_relocs(h, rela, relasz) != 0) {
                munmap(map, map_size);
                free(h);
                close(fd);
                return NULL;
            }
        }
    }

    close(fd);

    h->next = dl_list;
    dl_list = h;

    return h;
}

/* Look up the address of the named symbol in the given handle. */
void *dlsym(void *handle, const char *symbol)
{
    struct dl_handle *h = handle;
    if (!h || !h->symtab || !h->strtab)
        return NULL;
    for (size_t i = 0; i < h->nsyms; i++) {
        const char *name = h->strtab + h->symtab[i].st_name;
        if (strcmp(name, symbol) == 0)
            return (char *)h->base + h->symtab[i].st_value;
    }
    return NULL;
}

/* Unload the shared object referenced by handle. */
int dlclose(void *handle)
{
    struct dl_handle *h = handle;
    if (!h)
        return -1;
    struct dl_handle **pp = &dl_list;
    while (*pp && *pp != h)
        pp = &(*pp)->next;
    if (*pp == h)
        *pp = h->next;
    munmap(h->mapping, h->map_size);
    free(h->path);
    free(h);
    return 0;
}

/* Query symbol and object information for an address. */
int dladdr(void *addr, Dl_info *info)
{
    if (!info)
        return 0;

    for (struct dl_handle *h = dl_list; h; h = h->next) {
        char *start = h->base;
        char *end = (char *)h->base + h->map_size;
        if ((char *)addr >= start && (char *)addr < end) {
            info->dli_fname = h->path;
            info->dli_fbase = h->base;
            info->dli_sname = NULL;
            info->dli_saddr = NULL;
            if (h->symtab && h->strtab) {
                for (size_t i = 0; i < h->nsyms; i++) {
                    Elf64_Addr val = h->symtab[i].st_value;
                    if (val == 0)
                        continue;
                    void *sym_addr = (char *)h->base + val;
                    if (sym_addr == addr) {
                        info->dli_sname = h->strtab + h->symtab[i].st_name;
                        info->dli_saddr = sym_addr;
                        break;
                    }
                }
            }
            return 1;
        }
    }
    return 0;
}
