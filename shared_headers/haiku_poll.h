/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_POLL_H__
#define __HAIKU_POLL_H__

typedef unsigned long haiku_nfds_t;

struct haiku_pollfd
{
    int fd;
    short events;  /* events to look for */
    short revents; /* events that occured */
};

/* events & revents - compatible with the B_SELECT_xxx definitions in Drivers.h */
#define HAIKU_POLLIN      0x0001  /* any readable data available */
#define HAIKU_POLLOUT     0x0002  /* file descriptor is writeable */
#define HAIKU_POLLRDNORM  HAIKU_POLLIN
#define HAIKU_POLLWRNORM  HAIKU_POLLOUT
#define HAIKU_POLLRDBAND  0x0008  /* priority readable data */
#define HAIKU_POLLWRBAND  0x0010  /* priority data can be written */
#define HAIKU_POLLPRI     0x0020  /* high priority readable data */

/* revents only */
#define HAIKU_POLLERR     0x0004  /* errors pending */
#define HAIKU_POLLHUP     0x0080  /* disconnected */
#define HAIKU_POLLNVAL    0x1000  /* invalid file descriptor */

#endif /* __HAIKU_POLL_H__ */