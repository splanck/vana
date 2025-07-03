/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for simple stream I/O.
 */
#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#include <stdatomic.h>

typedef struct {
    int fd;                      /* underlying file descriptor */
    unsigned char *buf;          /* optional I/O buffer */
    size_t bufsize;              /* size of the buffer */
    size_t bufpos;               /* current read/write position in buf */
    size_t buflen;               /* valid data length in buf */
    int buf_owned;               /* buffer should be freed on close */
    int error;                   /* error indicator */
    int eof;                     /* end-of-file indicator */
    int have_ungot;              /* ungetc() character available */
    unsigned char ungot_char;    /* character from ungetc() */
    int is_mem;                  /* stream operates on memory rather than fd */
    int is_wmem;                 /* stream stores wchar_t instead of bytes */
    void **mem_bufp;             /* pointer to buffer pointer for mem streams */
    size_t *mem_sizep;           /* pointer to size for mem streams */
    int readable;                /* stream opened for reading */
    int writable;                /* stream opened for writing */
    int append;                  /* writes should append */
    int is_cookie;               /* stream uses user callbacks */
    void *cookie;                /* opaque user data */
    ssize_t (*cookie_read)(void *, char *, size_t);
    ssize_t (*cookie_write)(void *, const char *, size_t);
    int (*cookie_seek)(void *, off_t *, int);
    int (*cookie_close)(void *);
    atomic_flag lock;            /* for flockfile */
} FILE;

typedef off_t fpos_t;

typedef struct {
    ssize_t (*read)(void *, char *, size_t);
    ssize_t (*write)(void *, const char *, size_t);
    int (*seek)(void *, off_t *, int);
    int (*close)(void *);
} cookie_io_functions_t;

/*
 * A FILE may maintain an internal buffer for efficiency. Data written with
 * fwrite() accumulates in the buffer until it is full or fflush() is called.
 * Reads fill the buffer from the descriptor and bytes are consumed from bufpos
 * until empty. flush_buffer() in stdio.c performs the actual write back to the
 * descriptor or memory region when the buffer needs to be drained.
 */

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
#ifndef BUFSIZ
#define BUFSIZ 1024
#endif
#ifndef L_tmpnam
#define L_tmpnam 20
#endif

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen(const char *path, const char *mode);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fclose(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
int fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *stream);
int fgetpos(FILE *stream, fpos_t *pos);
int fsetpos(FILE *stream, const fpos_t *pos);
void rewind(FILE *stream);
int fgetc(FILE *stream);
int fputc(int c, FILE *stream);
int ungetc(int c, FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int fputs(const char *s, FILE *stream);
int fflush(FILE *stream);
int setvbuf(FILE *stream, char *buf, int mode, size_t size);
void setbuf(FILE *stream, char *buf);
int feof(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);
int fileno(FILE *stream);
FILE *freopen(const char *path, const char *mode, FILE *stream);
FILE *fdopen(int fd, const char *mode);

int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int sprintf(char *str, const char *format, ...);
int asprintf(char **strp, const char *format, ...);
int vasprintf(char **strp, const char *format, va_list ap);
int dprintf(int fd, const char *format, ...);
int vdprintf(int fd, const char *format, va_list ap);
int vprintf(const char *format, va_list ap);
int vfprintf(FILE *stream, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int vsprintf(char *str, const char *format, va_list ap);
/*
 * The scanf family supports %d, %u, %x, %o, %s, and floating point
 * formats like %f, %lf, and %g.
 */
int vscanf(const char *format, va_list ap);
int vfscanf(FILE *stream, const char *format, va_list ap);
int vsscanf(const char *str, const char *format, va_list ap);
int scanf(const char *format, ...);
int fscanf(FILE *stream, const char *format, ...);
int sscanf(const char *str, const char *format, ...);

/* Basic error helpers */
char *strerror(int errnum);
int strerror_r(int errnum, char *buf, size_t buflen);
void perror(const char *s);

FILE *popen(const char *command, const char *mode);
int pclose(FILE *stream);

int mkstemp(char *template);
int mkostemp(char *template, int flags);
int mkostemps(char *template, int suffixlen, int flags);
FILE *tmpfile(void);
/* Buffer must hold at least L_tmpnam characters or be NULL */
char *__tmpnam_chk(char *s, size_t sz);
char *__tmpnam_impl(char *s);
#ifdef __GNUC__
#define tmpnam(s) (__builtin_object_size((s), 0) != (size_t)-1 \
        ? __tmpnam_chk((s), __builtin_object_size((s), 0)) \
        : __tmpnam_impl(s))
#else
char *tmpnam(char *s);
#endif
char *tempnam(const char *dir, const char *pfx);

FILE *open_memstream(char **bufp, size_t *sizep);
FILE *open_wmemstream(wchar_t **bufp, size_t *sizep);
FILE *fmemopen(void *buf, size_t size, const char *mode);
FILE *fopencookie(void *cookie, const char *mode,
                  cookie_io_functions_t functions);
FILE *funopen(const void *cookie,
              int (*readfn)(void *, char *, int),
              int (*writefn)(void *, const char *, int),
              fpos_t (*seekfn)(void *, fpos_t, int),
              int (*closefn)(void *));

ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);

void flockfile(FILE *stream);
int ftrylockfile(FILE *stream);
void funlockfile(FILE *stream);

#endif /* STDIO_H */
