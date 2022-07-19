/*
 * Copyright 2006-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_IOCTL_H__
#define __HAIKU_IOCTL_H__


#include <haiku_termios.h>

/* These only work on sockets for now */
#define HAIKU_FIONBIO     0xbe000000
#define HAIKU_FIONREAD    0xbe000001

#define HAIKU_FIOSEEKDATA 0xbe000002
#define HAIKU_FIOSEEKHOLE 0xbe000003

#endif    /* __HAIKU_IOCTL_H__ */