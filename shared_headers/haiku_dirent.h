/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_DIRENT_H__
#define __HAIKU_DIRENT_H__

#include "BeDefs.h"

typedef struct haiku_dirent
{
    haiku_dev_t d_dev;              /* device */
    haiku_dev_t d_pdev;             /* parent device (only for queries) */
    haiku_ino_t d_ino;              /* inode number */
    haiku_ino_t d_pino;             /* parent inode (only for queries) */
    unsigned short d_reclen;        /* length of this record, not the name */
#if __GNUC__ == 2
    char d_name[0];                 /* name of the entry (null byte terminated) */
#else
    char d_name[];                  /* name of the entry (null byte terminated) */
#endif
} haiku_dirent_t;

#ifndef HAIKU_MAXNAMLEN
#   ifdef HAIKU_NAME_MAX
#       define HAIKU_MAXNAMLEN HAIKU_NAME_MAX
#   else
#       define HAIKU_MAXNAMLEN 256
#   endif
#endif

#endif /* _DIRENT_H */