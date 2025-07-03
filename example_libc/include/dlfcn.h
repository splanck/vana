/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for runtime loading of shared libraries.
 */
#ifndef DLFCN_H
#define DLFCN_H

#define RTLD_LAZY   1
#define RTLD_NOW    2
#define RTLD_LOCAL  0
#define RTLD_GLOBAL 0x100

void *dlopen(const char *filename, int flag);
void *dlsym(void *handle, const char *symbol);
int dlclose(void *handle);
const char *dlerror(void);

typedef struct {
    const char *dli_fname; /* Pathname of shared object */
    void *dli_fbase;       /* Base address where object is loaded */
    const char *dli_sname; /* Name of symbol closest to addr */
    void *dli_saddr;       /* Exact address of that symbol */
} Dl_info;

int dladdr(void *addr, Dl_info *info);

#endif /* DLFCN_H */
