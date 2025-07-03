/*
 * BSD 2-Clause License
 *
 * Purpose: Socket APIs and ancillary data helpers.
 */
#ifndef SYS_SOCKET_H
#define SYS_SOCKET_H

#include <sys/types.h>
#include <stddef.h>

#if defined(__has_include)
#  if __has_include("/usr/include/x86_64-linux-gnu/sys/socket.h")
#    include "/usr/include/x86_64-linux-gnu/sys/socket.h"
#    define VLIBC_SYS_SOCKET_NATIVE 1
#  elif __has_include("/usr/include/sys/socket.h")
#    include "/usr/include/sys/socket.h"
#    define VLIBC_SYS_SOCKET_NATIVE 1
#  endif
#endif

/* Provide basic message header structures when missing. */
#ifndef VLIBC_SYS_SOCKET_NATIVE
struct msghdr {
    void *msg_name;
    socklen_t msg_namelen;
    struct iovec *msg_iov;
    size_t msg_iovlen;
    void *msg_control;
    size_t msg_controllen;
    int msg_flags;
};

struct cmsghdr {
    size_t cmsg_len;
    int cmsg_level;
    int cmsg_type;
};

#define CMSG_ALIGN(len) (((len) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1))
#define CMSG_DATA(cmsg) ((unsigned char *)(cmsg) + CMSG_ALIGN(sizeof(struct cmsghdr)))
#define CMSG_SPACE(len) (CMSG_ALIGN(len) + CMSG_ALIGN(sizeof(struct cmsghdr)))
#define CMSG_LEN(len)   (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#define CMSG_FIRSTHDR(mhdr) \
    ((mhdr)->msg_controllen >= sizeof(struct cmsghdr) ? \
     (struct cmsghdr *)(mhdr)->msg_control : (struct cmsghdr *)0)
/*
 * Return the next cmsghdr in the ancillary data buffer or NULL when no
 * further headers remain.
 */
static inline struct cmsghdr *CMSG_NXTHDR(struct msghdr *mhdr, struct cmsghdr *cmsg)
{
    size_t pos = (char *)cmsg - (char *)mhdr->msg_control;
    size_t next = pos + CMSG_ALIGN(cmsg->cmsg_len);
    if (next + sizeof(struct cmsghdr) > mhdr->msg_controllen)
        return NULL;
    return (struct cmsghdr *)((char *)mhdr->msg_control + next);
}
#endif

/* Socket API wrappers */
int socket(int domain, int type, int protocol);
/* Create a new endpoint for communication. */
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
/* Associate a socket with a local address. */
int listen(int sockfd, int backlog);
/* Mark the socket as passive to accept incoming connections. */
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
/* Accept an incoming connection on a listening socket. */
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
/* Accept a connection and apply the given flags atomically. */
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
/* Initiate a connection on a socket. */
int socketpair(int domain, int type, int protocol, int sv[2]);
/* Create a pair of connected sockets. */
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
/* Send data on a connected socket. */
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
/* Receive data from a connected socket. */
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest, socklen_t addrlen);
/* Send a message to a specific destination. */
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src, socklen_t *addrlen);
/* Receive a message from a specific source. */
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
/* Query the local address of a socket. */
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
/* Query the remote address of a connected socket. */
int shutdown(int sockfd, int how);
/* Disable sends and/or receives on a socket. */
int setsockopt(int sockfd, int level, int optname,
               const void *optval, socklen_t optlen);
/* Set options on a socket. */
int getsockopt(int sockfd, int level, int optname,
               void *optval, socklen_t *optlen);
/* Retrieve options from a socket. */
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
/* Send a message with ancillary data. */
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
/* Receive a message along with ancillary data. */

#endif /* SYS_SOCKET_H */
