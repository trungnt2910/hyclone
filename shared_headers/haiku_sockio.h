/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_SYS_SOCKIO_H__
#define __HAIKU_SYS_SOCKIO_H__

/*! Socket I/O control codes, usually via struct ifreq, most of them should
    be compatible with the BSDs.
*/

#define HAIKU_SIOCADDRT 8900      /* add route */
#define HAIKU_SIOCDELRT 8901      /* delete route */
#define HAIKU_SIOCSIFADDR 8902    /* set interface address */
#define HAIKU_SIOCGIFADDR 8903    /* get interface address */
#define HAIKU_SIOCSIFDSTADDR 8904 /* set point-to-point address */
#define HAIKU_SIOCGIFDSTADDR 8905 /* get point-to-point address */
#define HAIKU_SIOCSIFFLAGS 8906   /* set interface flags */
#define HAIKU_SIOCGIFFLAGS 8907   /* get interface flags */
#define HAIKU_SIOCGIFBRDADDR 8908 /* get broadcast address */
#define HAIKU_SIOCSIFBRDADDR 8909 /* set broadcast address */
#define HAIKU_SIOCGIFCOUNT 8910   /* count interfaces */
#define HAIKU_SIOCGIFCONF 8911    /* get interface list */
#define HAIKU_SIOCGIFINDEX 8912   /* interface name -> index */
#define HAIKU_SIOCGIFNAME 8913    /* interface index -> name */
#define HAIKU_SIOCGIFNETMASK 8914 /* get net address mask */
#define HAIKU_SIOCSIFNETMASK 8915 /* set net address mask */
#define HAIKU_SIOCGIFMETRIC 8916  /* get interface metric */
#define HAIKU_SIOCSIFMETRIC 8917  /* set interface metric */
#define HAIKU_SIOCDIFADDR 8918    /* delete interface address */
#define HAIKU_SIOCAIFADDR 8919
/* configure interface alias, ifaliasreq */
#define HAIKU_SIOCADDMULTI 8920 /* add multicast address */
#define HAIKU_SIOCDELMULTI 8921 /* delete multicast address */
#define HAIKU_SIOCGIFMTU 8922   /* get interface MTU */
#define HAIKU_SIOCSIFMTU 8923   /* set interface MTU */
#define HAIKU_SIOCSIFMEDIA 8924 /* set net media */
#define HAIKU_SIOCGIFMEDIA 8925 /* get net media */

#define HAIKU_SIOCGRTSIZE 8926  /* get route table size */
#define HAIKU_SIOCGRTTABLE 8927 /* get route table */
#define HAIKU_SIOCGETRT 8928
/* get route information for destination */

#define HAIKU_SIOCGIFSTATS 8929 /* get interface stats */
#define HAIKU_SIOCGIFTYPE 8931  /* get interface type */

#define HAIKU_SIOCSPACKETCAP 8932
/* Start capturing packets on an interface */
#define HAIKU_SIOCCPACKETCAP 8933
/* Stop capturing packets on an interface */

#define HAIKU_SIOCSHIWAT 8934 /* set high watermark */
#define HAIKU_SIOCGHIWAT 8935 /* get high watermark */
#define HAIKU_SIOCSLOWAT 8936 /* set low watermark */
#define HAIKU_SIOCGLOWAT 8937 /* get low watermark */
#define HAIKU_SIOCATMARK 8938 /* at out-of-band mark? */
#define HAIKU_SIOCSPGRP 8939  /* set process group */
#define HAIKU_SIOCGPGRP 8940  /* get process group */

#define HAIKU_SIOCGPRIVATE_0 8941 /* device private 0 */
#define HAIKU_SIOCGPRIVATE_1 8942 /* device private 1 */
#define HAIKU_SIOCSDRVSPEC 8943   /* set driver-specific parameters */
#define HAIKU_SIOCGDRVSPEC 8944   /* get driver-specific parameters */

#define HAIKU_SIOCSIFGENERIC 8945 /* generic IF set op */
#define HAIKU_SIOCGIFGENERIC 8946 /* generic IF get op */

/* Haiku specific extensions */
#define B_SOCKET_REMOVE_ALIAS 8918  /* synonym for SIOCDIFADDR */
#define B_SOCKET_ADD_ALIAS 8919     /* synonym for SIOCAIFADDR */
#define B_SOCKET_SET_ALIAS 8947     /* set interface alias, ifaliasreq */
#define B_SOCKET_GET_ALIAS 8948     /* get interface alias, ifaliasreq */
#define B_SOCKET_COUNT_ALIASES 8949 /* count interface aliases */

#define HAIKU_SIOCEND 9000 /* SIOCEND >= highest SIOC* */

#endif /* __HAIKU_SYS_SOCKIO_H__ */