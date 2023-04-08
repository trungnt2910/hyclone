/*
 * Copyright 2006-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_NET_ROUTE_H__
#define __HAIKU_NET_ROUTE_H__

#include "haiku_socket.h"

#define HAIKU_RTF_UP 0x00000001
#define HAIKU_RTF_GATEWAY 0x00000002
#define HAIKU_RTF_HOST 0x00000004
#define HAIKU_RTF_REJECT 0x00000008
#define HAIKU_RTF_DYNAMIC 0x00000010
#define HAIKU_RTF_MODIFIED 0x00000020
#define HAIKU_RTF_DEFAULT 0x00000080
#define HAIKU_RTF_STATIC 0x00000800
#define HAIKU_RTF_BLACKHOLE 0x00001000
#define HAIKU_RTF_LOCAL 0x00200000

/* This structure is used to pass routes to and from the network stack
 * (via struct ifreq) */

struct haiku_route_entry
{
    struct haiku_sockaddr *destination;
    struct haiku_sockaddr *mask;
    struct haiku_sockaddr *gateway;
    struct haiku_sockaddr *source;
    uint32_t flags;
    uint32_t mtu;
};

#endif /* __HAIKU_NET_ROUTE_H__ */