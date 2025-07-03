/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for standard error codes.
 */
#ifndef ERRNO_H
#define ERRNO_H

/* Standard error codes */
#define EPERM            1
#define ENOENT           2
#define ESRCH            3
#define EINTR            4
#define EIO              5
#define ENXIO            6
#define E2BIG            7
#define ENOEXEC          8
#define EBADF            9
#define ECHILD           10
#define EAGAIN           11
#define ENOMEM           12
#define EACCES           13
#define EFAULT           14
#define ENOTBLK          15
#define EBUSY            16
#define EEXIST           17
#define EXDEV            18
#define ENODEV           19
#define ENOTDIR          20
#define EISDIR           21
#define EINVAL           22
#define ENFILE           23
#define EMFILE           24
#define ENOTTY           25
#define ETXTBSY          26
#define EFBIG            27
#define ENOSPC           28
#define ESPIPE           29
#define EROFS            30
#define EMLINK           31
#define EPIPE            32
#define EDOM             33
#define ERANGE           34
#define EDEADLK          35
#define ENAMETOOLONG     36
#define ENOLCK           37
#define ENOSYS           38
#define ENOTEMPTY        39
#define ELOOP            40
#define EWOULDBLOCK      EAGAIN
#define EDEADLOCK        EDEADLK
#define ENOMSG           42
#define EIDRM            43
#define EREMOTE          71
#define ESHUTDOWN        58
#define ETOOMANYREFS     59
#define EHOSTDOWN        64
#define EPROCLIM         67
#define EUSERS           68
#define EDQUOT           69
#define EBADRPC          72
#define ERPCMISMATCH     73
#define EPROGUNAVAIL     74
#define EPROGMISMATCH    75
#define EPROCUNAVAIL     76
#define EFTYPE           79
#define EAUTH            80
#define ENEEDAUTH        81
#define EOVERFLOW        84
#define EILSEQ           86
#define ENOATTR          87
#define EDOOFUS          88
#define ENOSTR           60
#define ENODATA          61
#define ETIME            62
#define ENOSR            63
#define EBADMSG          89
#define EMULTIHOP        90
#define ENOLINK          91
#define EPROTO           92
#define ENOTCAPABLE      93
#define ECAPMODE         94
#define ENOTRECOVERABLE  95
#define EOWNERDEAD       96
#define EINTEGRITY       97
#define ENOTSOCK         88
#define EDESTADDRREQ     89
#define EMSGSIZE         90
#define EPROTOTYPE       91
#define ENOPROTOOPT      92
#define EPROTONOSUPPORT  93
#define ESOCKTNOSUPPORT  94
#define EOPNOTSUPP       95
#define EPFNOSUPPORT     96
#define EAFNOSUPPORT     97
#define EADDRINUSE       98
#define EADDRNOTAVAIL    99
#define ENETDOWN         100
#define ENETUNREACH      101
#define ENETRESET        102
#define ECONNABORTED     103
#define ECONNRESET       104
#define ENOBUFS          105
#define EISCONN          106
#define ENOTCONN         107
#define ETIMEDOUT        110
#define ECONNREFUSED     111
#define EHOSTUNREACH     113
#define EALREADY         114
#define EINPROGRESS      115
#define ESTALE           116
#define ECANCELED        125

extern __thread int errno;
int *__errno_location(void);

#endif /* ERRNO_H */
