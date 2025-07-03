/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for user account database access.
 */
#ifndef PWD_H
#define PWD_H

#include <sys/types.h>
#include <stddef.h>

struct passwd {
    char *pw_name;
    char *pw_passwd;
    uid_t pw_uid;
    gid_t pw_gid;
    char *pw_gecos;
    char *pw_dir;
    char *pw_shell;
};

struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *name);
int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen,
               struct passwd **result);
int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t buflen,
               struct passwd **result);
void setpwent(void);
struct passwd *getpwent(void);
void endpwent(void);

#endif /* PWD_H */
