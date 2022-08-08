/*
 * Copyright 2002-2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_NETINET_IN_H__
#define __HAIKU_NETINET_IN_H__

#include <cstdint>
#include <haiku_socket.h>

typedef uint16_t haiku_in_port_t;
typedef uint32_t haiku_in_addr_t;

/* Protocol definitions */
#define HAIKU_IPPROTO_IP 0        /* 0, IPv4 */
#define HAIKU_IPPROTO_HOPOPTS 0   /* 0, IPv6 hop-by-hop options */
#define HAIKU_IPPROTO_ICMP 1      /* 1, ICMP (v4) */
#define HAIKU_IPPROTO_IGMP 2      /* 2, IGMP (group management) */
#define HAIKU_IPPROTO_TCP 6       /* 6, tcp */
#define HAIKU_IPPROTO_UDP 17      /* 17, UDP */
#define HAIKU_IPPROTO_IPV6 41     /* 41, IPv6 in IPv6 */
#define HAIKU_IPPROTO_ROUTING 43  /* 43, Routing */
#define HAIKU_IPPROTO_FRAGMENT 44 /* 44, IPv6 fragmentation header */
#define HAIKU_IPPROTO_ESP 50      /* 50, Encap Sec. Payload */
#define HAIKU_IPPROTO_AH 51       /* 51, Auth Header */
#define HAIKU_IPPROTO_ICMPV6 58   /* 58, IPv6 ICMP */
#define HAIKU_IPPROTO_NONE 59     /* 59, IPv6 no next header */
#define HAIKU_IPPROTO_DSTOPTS 60  /* 60, IPv6 destination option */
#define HAIKU_IPPROTO_ETHERIP 97  /* 97, Ethernet in IPv4 */
#define HAIKU_IPPROTO_RAW 255     /* 255 */

#define HAIKU_IPPROTO_MAX 256

/* Port numbers */

#define HAIKU_IPPORT_RESERVED 1024
/* < IPPORT_RESERVED are privileged and should be accessible only by root */
#define HAIKU_IPPORT_USERRESERVED 49151
/* > IPPORT_USERRESERVED are reserved for servers, though not requiring
 * privileged status
 */

/* IP Version 4 address */
struct haiku_in_addr
{
    haiku_in_addr_t s_addr;
};

/* IP Version 4 socket address */
struct haiku_sockaddr_in
{
    uint8_t sin_len;
    uint8_t sin_family;
    uint16_t sin_port;
    struct haiku_in_addr sin_addr;
    int8_t sin_zero[24];
};

/* RFC 3678 - Socket Interface Extensions for Multicast Source Filters */

struct haiku_ip_mreq
{
    struct haiku_in_addr imr_multiaddr; /* IP address of group */
    struct haiku_in_addr imr_interface; /* IP address of interface */
};

struct haiku_ip_mreq_source
{
    struct haiku_in_addr imr_multiaddr;  /* IP address of group */
    struct haiku_in_addr imr_sourceaddr; /* IP address of source */
    struct haiku_in_addr imr_interface;  /* IP address of interface */
};

struct haiku_group_req
{
    uint32_t gr_interface;            /* interface index */
    struct haiku_sockaddr_storage gr_group; /* group address */
};

struct haiku_group_source_req
{
    uint32_t gsr_interface;             /* interface index */
    struct haiku_sockaddr_storage gsr_group;  /* group address */
    struct haiku_sockaddr_storage gsr_source; /* source address */
};

/*
 * Options for use with [gs]etsockopt at the IP level.
 * First word of comment is data type; bool is stored in int.
 */
#define HAIKU_IP_OPTIONS 1 /* buf/ip_opts; set/get IP options */
#define HAIKU_IP_HDRINCL 2 /* int; header is included with data */
#define HAIKU_IP_TOS 3
/* int; IP type of service and preced. */
#define HAIKU_IP_TTL 4            /* int; IP time to live */
#define HAIKU_IP_RECVOPTS 5       /* bool; receive all IP opts w/dgram */
#define HAIKU_IP_RECVRETOPTS 6    /* bool; receive IP opts for response */
#define HAIKU_IP_RECVDSTADDR 7    /* bool; receive IP dst addr w/dgram */
#define HAIKU_IP_RETOPTS 8        /* ip_opts; set/get IP options */
#define HAIKU_IP_MULTICAST_IF 9   /* in_addr; set/get IP multicast i/f  */
#define HAIKU_IP_MULTICAST_TTL 10 /* u_char; set/get IP multicast ttl */
#define HAIKU_IP_MULTICAST_LOOP 11
/* u_char; set/get IP multicast loopback */
#define HAIKU_IP_ADD_MEMBERSHIP 12
/* ip_mreq; add an IP group membership */
#define HAIKU_IP_DROP_MEMBERSHIP 13
/* ip_mreq; drop an IP group membership */
#define HAIKU_IP_BLOCK_SOURCE 14           /* ip_mreq_source */
#define HAIKU_IP_UNBLOCK_SOURCE 15         /* ip_mreq_source */
#define HAIKU_IP_ADD_SOURCE_MEMBERSHIP 16  /* ip_mreq_source */
#define HAIKU_IP_DROP_SOURCE_MEMBERSHIP 17 /* ip_mreq_source */
#define HAIKU_MCAST_JOIN_GROUP 18          /* group_req */
#define HAIKU_MCAST_BLOCK_SOURCE 19        /* group_source_req */
#define HAIKU_MCAST_UNBLOCK_SOURCE 20      /* group_source_req */
#define HAIKU_MCAST_LEAVE_GROUP 21         /* group_req */
#define HAIKU_MCAST_JOIN_SOURCE_GROUP 22   /* group_source_req */
#define HAIKU_MCAST_LEAVE_SOURCE_GROUP 23  /* group_source_req */

/* IPPROTO_IPV6 options */
#define HAIKU_IPV6_MULTICAST_IF 24   /* int */
#define HAIKU_IPV6_MULTICAST_HOPS 25 /* int */
#define HAIKU_IPV6_MULTICAST_LOOP 26 /* int */

#define HAIKU_IPV6_UNICAST_HOPS 27 /* int */
#define HAIKU_IPV6_JOIN_GROUP 28   /* struct haiku_ipv6_mreq */
#define HAIKU_IPV6_LEAVE_GROUP 29  /* struct haiku_ipv6_mreq */
#define HAIKU_IPV6_V6ONLY 30       /* int */

#define HAIKU_IPV6_PKTINFO 31      /* struct haiku_ipv6_pktinfo */
#define HAIKU_IPV6_RECVPKTINFO 32  /* struct haiku_ipv6_pktinfo */
#define HAIKU_IPV6_HOPLIMIT 33     /* int */
#define HAIKU_IPV6_RECVHOPLIMIT 34 /* int */

#define HAIKU_IPV6_HOPOPTS 35 /* struct haiku_ip6_hbh */
#define HAIKU_IPV6_DSTOPTS 36 /* struct haiku_ip6_dest */
#define HAIKU_IPV6_RTHDR 37   /* struct haiku_ip6_rthdr */

#define HAIKU_INADDR_ANY ((in_addr_t)0x00000000)
#define HAIKU_INADDR_LOOPBACK ((in_addr_t)0x7f000001)
#define HAIKU_INADDR_BROADCAST ((in_addr_t)0xffffffff) /* must be masked */

#define HAIKU_INADDR_UNSPEC_GROUP ((in_addr_t)0xe0000000)     /* 224.0.0.0 */
#define HAIKU_INADDR_ALLHOSTS_GROUP ((in_addr_t)0xe0000001)   /* 224.0.0.1 */
#define HAIKU_INADDR_ALLROUTERS_GROUP ((in_addr_t)0xe0000002) /* 224.0.0.2 */
#define HAIKU_INADDR_MAX_LOCAL_GROUP ((in_addr_t)0xe00000ff)  /* 224.0.0.255 */

#define HAIKU_IN_LOOPBACKNET 127

#define HAIKU_INADDR_NONE ((in_addr_t)0xffffffff)

#define HAIKU_IN_CLASSA(i) (((in_addr_t)(i)&0x80000000) == 0)
#define HAIKU_IN_CLASSA_NET 0xff000000
#define HAIKU_IN_CLASSA_NSHIFT 24
#define HAIKU_IN_CLASSA_HOST 0x00ffffff
#define HAIKU_IN_CLASSA_MAX 128

#define HAIKU_IN_CLASSB(i) (((in_addr_t)(i)&0xc0000000) == 0x80000000)
#define HAIKU_IN_CLASSB_NET 0xffff0000
#define HAIKU_IN_CLASSB_NSHIFT 16
#define HAIKU_IN_CLASSB_HOST 0x0000ffff
#define HAIKU_IN_CLASSB_MAX 65536

#define HAIKU_IN_CLASSC(i) (((in_addr_t)(i)&0xe0000000) == 0xc0000000)
#define HAIKU_IN_CLASSC_NET 0xffffff00
#define HAIKU_IN_CLASSC_NSHIFT 8
#define HAIKU_IN_CLASSC_HOST 0x000000ff

#define HAIKU_IN_CLASSD(i) (((in_addr_t)(i)&0xf0000000) == 0xe0000000)
/* These ones aren't really net and host fields, but routing needn't know. */
#define HAIKU_IN_CLASSD_NET 0xf0000000
#define HAIKU_IN_CLASSD_NSHIFT 28
#define HAIKU_IN_CLASSD_HOST 0x0fffffff

#define HAIKU_IN_MULTICAST(i) IN_CLASSD(i)

#define HAIKU_IN_EXPERIMENTAL(i) (((in_addr_t)(i)&0xf0000000) == 0xf0000000)
#define HAIKU_IN_BADCLASS(i) (((in_addr_t)(i)&0xf0000000) == 0xf0000000)

#define HAIKU_IP_MAX_MEMBERSHIPS 20

/* maximal length of the string representation of an IPv4 address */
#define HAIKU_INET_ADDRSTRLEN 16

/* some helpful macro's :) */
#define HAIKU_in_hosteq(s, t) ((s).s_addr == (t).s_addr)
#define HAIKU_in_nullhost(x) ((x).s_addr == INADDR_ANY)
#define HAIKU_satosin(sa) ((struct haiku_sockaddr_in *)(sa))
#define HAIKU_sintosa(sin) ((struct haiku_sockaddr *)(sin))

struct haiku_in6_addr
{
    union
    {
        uint8_t                 u6_addr8[16];
        uint16_t                u6_addr16[8];
        uint32_t                u6_addr32[4];
    } in6_u;
    #define haiku_s6_addr       in6_u.u6_addr8
    #define haiku_s6_addr16     in6_u.u6_addr16
    #define haiku_s6_addr32     in6_u.u6_addr32
};

/* IP Version 6 socket address. */
struct haiku_sockaddr_in6
{
    uint8_t                     sin6_len;
    uint8_t                     sin6_family;
    uint16_t                    sin6_port;
    uint32_t                    sin6_flowinfo;
    struct haiku_in6_addr       sin6_addr;
    uint32_t                    sin6_scope_id;
};

#endif /* _NETINET_IN_H_ */