/*
 * Copyright 2006-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_NET_IF_H__
#define __HAIKU_NET_IF_H__

#include "haiku_net_route.h"
#include "haiku_socket.h"

#define HAIKU_IF_NAMESIZE 32

/* BSD specific/proprietary part */

#define HAIKU_IFNAMSIZ HAIKU_IF_NAMESIZE

struct haiku_ifreq_stream_stats
{
    uint32_t packets;
    uint32_t errors;
    uint64_t bytes;
    uint32_t multicast_packets;
    uint32_t dropped;
};

struct haiku_ifreq_stats
{
    struct haiku_ifreq_stream_stats receive;
    struct haiku_ifreq_stream_stats send;
    uint32_t collisions;
};

struct haiku_ifreq
{
    char ifr_name[HAIKU_IF_NAMESIZE];
    union
    {
        struct haiku_sockaddr ifr_addr;
        struct haiku_sockaddr ifr_dstaddr;
        struct haiku_sockaddr ifr_broadaddr;
        struct haiku_sockaddr ifr_mask;
        struct haiku_ifreq_stats ifr_stats;
        struct haiku_route_entry ifr_route;
        int ifr_flags;
        int ifr_index;
        int ifr_metric;
        int ifr_mtu;
        int ifr_media;
        int ifr_type;
        int ifr_reqcap;
        int ifr_count;
        uint8_t *ifr_data;
    };
};

/* used with SIOC_IF_ALIAS_ADD, SIOC_IF_ALIAS_GET, SIOC_ALIAS_SET */
struct haiku_ifaliasreq
{
    char ifra_name[HAIKU_IF_NAMESIZE];
    int ifra_index;
    struct haiku_sockaddr_storage ifra_addr;
    union
    {
        struct haiku_sockaddr_storage ifra_broadaddr;
        struct haiku_sockaddr_storage ifra_destination;
    };
    struct haiku_sockaddr_storage ifra_mask;
    uint32_t ifra_flags;
};

/* interface flags */
#define HAIKU_IFF_UP 0x0001
#define HAIKU_IFF_BROADCAST 0x0002 /* valid broadcast address */
#define HAIKU_IFF_LOOPBACK 0x0008
#define HAIKU_IFF_POINTOPOINT 0x0010 /* point-to-point link */
#define HAIKU_IFF_NOARP 0x0040       /* no address resolution */
#define HAIKU_IFF_AUTOUP 0x0080      /* auto dial */
#define HAIKU_IFF_PROMISC 0x0100     /* receive all packets */
#define HAIKU_IFF_ALLMULTI 0x0200    /* receive all multicast packets */
#define HAIKU_IFF_SIMPLEX 0x0800     /* doesn't receive own transmissions */
#define HAIKU_IFF_LINK 0x1000        /* has link */
#define HAIKU_IFF_AUTO_CONFIGURED 0x2000
#define HAIKU_IFF_CONFIGURING 0x4000
#define HAIKU_IFF_MULTICAST 0x8000 /* supports multicast */

/* interface alias flags */
#define HAIKU_IFAF_AUTO_CONFIGURED 0x0001 /* has been automatically configured */
#define HAIKU_IFAF_CONFIGURING 0x0002     /* auto configuration in progress */

/* used with SIOCGIFCOUNT, and SIOCGIFCONF */
struct haiku_ifconf
{
    int ifc_len; /* size of buffer */
    union
    {
        void *ifc_buf;
        struct haiku_ifreq *ifc_req;
        int ifc_value;
    };
};

/* POSIX definitions follow */

struct haiku_if_nameindex
{
    unsigned if_index; /* positive interface index */
    char *if_name;     /* interface name, ie. "loopback" */
};

#endif /* __HAIKU_NET_IF_H__ */