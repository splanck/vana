/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for environment variable access.
 */
#ifndef ENV_H
#define ENV_H

#include <stddef.h>

/* Access to environment variables */

/* global environment pointer */
extern char **environ;

/* initialize environment from envp */
void env_init(char **envp);

char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int putenv(const char *str);
int unsetenv(const char *name);
int clearenv(void);

/* Host name helpers */
int gethostname(char *name, size_t len);
int sethostname(const char *name, size_t len);

#endif /* ENV_H */
