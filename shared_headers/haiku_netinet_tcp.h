/*
 * Copyright 2006-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_NETINET_TCP_H__
#define __HAIKU_NETINET_TCP_H__

#include <stdint.h>

struct haiku_tcphdr
{
    uint16_t th_sport; /* source port */
    uint16_t th_dport; /* destination port */
    uint32_t th_seq;
    uint32_t th_ack;

#if BIG_ENDIAN
    uint8_t th_off : 4; /* data offset */
    uint8_t th_x2 : 4;  /* unused */
#else
    uint8_t th_x2 : 4;
    uint8_t th_off : 4;
#endif
    uint8_t th_flags;
    uint16_t th_win;
    uint16_t th_sum;
    uint16_t th_urp; /* end of urgent data offset */
} _PACKED;

#if 0
#define HAIKU_TH_FIN 0x01
#define HAIKU_TH_SYN 0x02
#define HAIKU_TH_RST 0x04
#define HAIKU_TH_PUSH 0x08
#define HAIKU_TH_ACK 0x10
#define HAIKU_TH_URG 0x20
#define HAIKU_TH_ECE 0x40
#define HAIKU_TH_CWR 0x80

#define HAIKU_TCPOPT_EOL 0
#define HAIKU_TCPOPT_NOP 1
#define HAIKU_TCPOPT_MAXSEG 2
#define HAIKU_TCPOPT_WINDOW 3
#define HAIKU_TCPOPT_SACK_PERMITTED 4
#define HAIKU_TCPOPT_SACK 5
#define HAIKU_TCPOPT_TIMESTAMP 8
#define HAIKU_TCPOPT_SIGNATURE 19

#define HAIKU_MAX_TCPOPTLEN 40
	/* absolute maximum TCP options length */

#define HAIKU_TCPOLEN_MAXSEG 4
#define HAIKU_TCPOLEN_WINDOW 3
#define HAIKU_TCPOLEN_SACK 8
#define HAIKU_TCPOLEN_SACK_PERMITTED 2
#define HAIKU_TCPOLEN_TIMESTAMP 10
#define HAIKU_TCPOLEN_TSTAMP_APPA (TCPOLEN_TIMESTAMP + 2)
	/* see RFC 1323, appendix A */
#define HAIKU_TCPOLEN_SIGNATURE 18

#define HAIKU_TCPOPT_TSTAMP_HDR \
    (TCPOPT_NOP << 24 | TCPOPT_NOP << 16 | TCPOPT_TIMESTAMP << 8 | TCPOLEN_TIMESTAMP)

#define HAIKU_TCP_MSS 536
#define HAIKU_TCP_MAXWIN 65535
#define HAIKU_TCP_MAX_WINSHIFT 14

#endif

/* options that can be set using setsockopt() and level IPPROTO_TCP */

#define HAIKU_TCP_NODELAY 0x01
/* don't delay sending smaller packets */
#define HAIKU_TCP_MAXSEG 0x02
/* set maximum segment size */
#define HAIKU_TCP_NOPUSH 0x04
/* don't use TH_PUSH */
#define HAIKU_TCP_NOOPT 0x08
/* don't use any TCP options */

#endif /* __HAIKU_NETINET_TCP_H__ */