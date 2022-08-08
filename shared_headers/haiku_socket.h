/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_SYS_SOCKET_H__
#define __HAIKU_SYS_SOCKET_H__

#include <stdint.h>
#include "BeDefs.h"

typedef uint32_t haiku_socklen_t;
typedef uint8_t haiku_sa_family_t;

/* Address families */
#define HAIKU_AF_UNSPEC 0
#define HAIKU_AF_INET 1
#define HAIKU_AF_APPLETALK 2
#define HAIKU_AF_ROUTE 3
#define HAIKU_AF_LINK 4
#define HAIKU_AF_INET6 5
#define HAIKU_AF_DLI 6
#define HAIKU_AF_IPX 7
#define HAIKU_AF_NOTIFY 8
#define HAIKU_AF_LOCAL 9
#define HAIKU_AF_UNIX HAIKU_AF_LOCAL
#define HAIKU_AF_BLUETOOTH 10
#define HAIKU_AF_MAX 11

/* Protocol families, deprecated */
#define HAIKU_PF_UNSPEC HAIKU_AF_UNSPEC
#define HAIKU_PF_INET HAIKU_AF_INET
#define HAIKU_PF_ROUTE HAIKU_AF_ROUTE
#define HAIKU_PF_LINK HAIKU_AF_LINK
#define HAIKU_PF_INET6 HAIKU_AF_INET6
#define HAIKU_PF_LOCAL HAIKU_AF_LOCAL
#define HAIKU_PF_UNIX HAIKU_AF_UNIX
#define HAIKU_PF_BLUETOOTH HAIKU_AF_BLUETOOTH

/* Socket types */
#define HAIKU_SOCK_STREAM 1
#define HAIKU_SOCK_DGRAM 2
#define HAIKU_SOCK_RAW 3
#define HAIKU_SOCK_SEQPACKET 5
#define HAIKU_SOCK_MISC 255

/* Socket options for SOL_SOCKET level */
#define HAIKU_SOL_SOCKET -1

#define HAIKU_SO_ACCEPTCONN 0x00000001  /* socket has had listen() */
#define HAIKU_SO_BROADCAST 0x00000002   /* permit sending of broadcast msgs */
#define HAIKU_SO_DEBUG 0x00000004       /* turn on debugging info recording */
#define HAIKU_SO_DONTROUTE 0x00000008   /* just use interface addresses */
#define HAIKU_SO_KEEPALIVE 0x00000010   /* keep connections alive */
#define HAIKU_SO_OOBINLINE 0x00000020   /* leave received OOB data in line */
#define HAIKU_SO_REUSEADDR 0x00000040   /* allow local address reuse */
#define HAIKU_SO_REUSEPORT 0x00000080   /* allow local address & port reuse */
#define HAIKU_SO_USELOOPBACK 0x00000100 /* bypass hardware when possible */
#define HAIKU_SO_LINGER 0x00000200      /* linger on close if data present */

#define HAIKU_SO_SNDBUF 0x40000001   /* send buffer size */
#define HAIKU_SO_SNDLOWAT 0x40000002 /* send low-water mark */
#define HAIKU_SO_SNDTIMEO 0x40000003 /* send timeout */
#define HAIKU_SO_RCVBUF 0x40000004   /* receive buffer size */
#define HAIKU_SO_RCVLOWAT 0x40000005 /* receive low-water mark */
#define HAIKU_SO_RCVTIMEO 0x40000006 /* receive timeout */
#define HAIKU_SO_ERROR 0x40000007    /* get error status and clear */
#define HAIKU_SO_TYPE 0x40000008     /* get socket type */
#define HAIKU_SO_NONBLOCK 0x40000009
#define HAIKU_SO_BINDTODEVICE 0x4000000a /* binds the socket to a specific device index */
#define HAIKU_SO_PEERCRED 0x4000000b     /* get peer credentials, param: ucred */

/* Shutdown options */
#define HAIKU_SHUT_RD 0
#define HAIKU_SHUT_WR 1
#define HAIKU_SHUT_RDWR 2

#define HAIKU_SOMAXCONN 32 /* Max listen queue for a socket */

struct haiku_linger
{
    int l_onoff;
    int l_linger;
};

struct haiku_sockaddr
{
    uint8_t sa_len;
    haiku_sa_family_t sa_family;
    uint8_t sa_data[30];
};

struct haiku_sockaddr_storage
{
    uint8_t ss_len;         /* total length */
    haiku_sa_family_t ss_family;  /* address family */
    uint8_t __ss_pad1[6];   /* align to quad */
    uint64_t __ss_pad2;     /* force alignment to 64 bit */
    uint8_t __ss_pad3[112]; /* pad to a total of 128 bytes */
};

struct haiku_msghdr
{
    void *msg_name;           /* address we're using (optional) */
    haiku_socklen_t msg_namelen;    /* length of address */
    struct haiku_iovec *msg_iov;    /* scatter/gather array we'll use */
    int msg_iovlen;           /* # elements in msg_iov */
    void *msg_control;        /* extra data */
    haiku_socklen_t msg_controllen; /* length of extra data */
    int msg_flags;            /* flags */
};

/* Flags for the msghdr.msg_flags field */
#define HAIKU_MSG_OOB 0x0001       /* process out-of-band data */
#define HAIKU_MSG_PEEK 0x0002      /* peek at incoming message */
#define HAIKU_MSG_DONTROUTE 0x0004 /* send without using routing tables */
#define HAIKU_MSG_EOR 0x0008       /* data completes record */
#define HAIKU_MSG_TRUNC 0x0010     /* data discarded before delivery */
#define HAIKU_MSG_CTRUNC 0x0020    /* control data lost before delivery */
#define HAIKU_MSG_WAITALL 0x0040   /* wait for full request or error */
#define HAIKU_MSG_DONTWAIT 0x0080  /* this message should be nonblocking */
#define HAIKU_MSG_BCAST 0x0100     /* this message rec'd as broadcast */
#define HAIKU_MSG_MCAST 0x0200     /* this message rec'd as multicast */
#define HAIKU_MSG_EOF 0x0400       /* data completes connection */
#define HAIKU_MSG_NOSIGNAL 0x0800  /* don't raise SIGPIPE if socket is closed */

struct haiku_cmsghdr
{
    haiku_socklen_t cmsg_len;
    int cmsg_level;
    int cmsg_type;
    /* data follows */
};

/* cmsghdr access macros */
#define HAIKU_CMSG_DATA(cmsg) ((unsigned char *)(cmsg) + _ALIGN(sizeof(struct haiku_cmsghdr)))
#define HAIKU_CMSG_NXTHDR(mhdr, cmsg)                                                                                                          \
    (((char *)(cmsg) + _ALIGN((cmsg)->cmsg_len) + _ALIGN(sizeof(struct haiku_cmsghdr)) > (char *)(mhdr)->msg_control + (mhdr)->msg_controllen) \
         ? (struct haiku_cmsghdr *)NULL                                                                                                        \
         : (struct haiku_cmsghdr *)((char *)(cmsg) + _ALIGN((cmsg)->cmsg_len)))
#define HAIKU_CMSG_FIRSTHDR(mhdr)                           \
    ((mhdr)->msg_controllen >= sizeof(struct haiku_cmsghdr) \
         ? (struct haiku_cmsghdr *)(mhdr)->msg_control      \
         : (struct haiku_cmsghdr *)NULL)
#define HAIKU_CMSG_SPACE(len) (_ALIGN(sizeof(struct haiku_cmsghdr)) + _ALIGN(len))
#define HAIKU_CMSG_LEN(len) (_ALIGN(sizeof(struct haiku_cmsghdr)) + (len))
#define HAIKU_CMSG_ALIGN(len) _ALIGN(len)

/* SOL_SOCKET control message types */
#define HAIKU_SCM_RIGHTS 0x01

/* parameter to SO_PEERCRED */
struct haiku_ucred
{
    haiku_pid_t pid; /* PID of sender */
    haiku_uid_t uid; /* UID of sender */
    haiku_gid_t gid; /* GID of sender */
};

struct haiku_sockaddr_un
{
    uint8_t     sun_len;
    uint8_t     sun_family;
    char        sun_path[126];
};

#endif /* __HAIKU_SYS_SOCKET_H__ */