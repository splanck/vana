/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for basic utilities.
 */
#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>
#include <stdint.h>
#include "memory.h"

typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;

/* Environment variable access */
extern char **environ;
char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);
int clearenv(void);

/* Current working directory */
char *getcwd(char *buf, size_t size);
char *realpath(const char *path, char *resolved_path);

/* Path utilities */
char *basename(const char *path);
char *dirname(const char *path);
char *mkdtemp(char *template);

/* Execute a shell command */
int system(const char *command);

/* String conversion */
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
intmax_t strtoimax(const char *nptr, char **endptr, int base);
uintmax_t strtoumax(const char *nptr, char **endptr, int base);
int atoi(const char *nptr);
float strtof(const char *nptr, char **endptr);
double strtod(const char *nptr, char **endptr);
long double strtold(const char *nptr, char **endptr);
double atof(const char *nptr);
/* Length of next multibyte character */
int mblen(const char *s, size_t n);

int abs(int j);
long labs(long j);
long long llabs(long long j);

div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);


/* Sorting helpers */
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));
void qsort_r(void *base, size_t nmemb, size_t size,
             int (*compar)(const void *, const void *, void *), void *ctx);
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));

/* Pseudo-random number generator */
int rand(void);
void srand(unsigned seed);
unsigned int arc4random(void);
void arc4random_buf(void *buf, size_t len);
unsigned int arc4random_uniform(unsigned int upper_bound);
int rand_r(unsigned *state);
double drand48(void);
double erand48(unsigned short xseed[3]);
long lrand48(void);
long nrand48(unsigned short xseed[3]);
void srand48(long seedval);
unsigned short *seed48(unsigned short seed16v[3]);
void lcong48(unsigned short param[7]);

/* System load averages */
int getloadavg(double loadavg[], int nelem);

/* Register a function to run at normal process exit */
int atexit(void (*fn)(void));

int at_quick_exit(void (*func)(void));
void quick_exit(int status);

/* Terminate the process with SIGABRT */
void abort(void);

#endif /* STDLIB_H */
