/*
 * Copyright 2002-2016, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_FS_INFO_H__
#define __HAIKU_FS_INFO_H__

#include "BeDefs.h"

/* fs_info.flags */
#define B_FS_IS_READONLY                0x00000001
#define B_FS_IS_REMOVABLE               0x00000002
#define B_FS_IS_PERSISTENT              0x00000004
#define B_FS_IS_SHARED                  0x00000008
#define B_FS_HAS_MIME                   0x00010000
#define B_FS_HAS_ATTR                   0x00020000
#define B_FS_HAS_QUERY                  0x00040000
// those additions are preliminary and may be removed
#define B_FS_HAS_SELF_HEALING_LINKS     0x00080000
#define B_FS_HAS_ALIASES                0x00100000
#define B_FS_SUPPORTS_NODE_MONITORING   0x00200000
#define B_FS_SUPPORTS_MONITOR_CHILDREN  0x00400000

typedef struct haiku_fs_info
{
    haiku_dev_t     dev;                                /* volume dev_t */
    haiku_ino_t     root;                               /* root ino_t */
    uint32          flags;                              /* flags (see above) */
    haiku_off_t     block_size;                         /* fundamental block size */
    haiku_off_t     io_size;                            /* optimal i/o size */
    haiku_off_t     total_blocks;                       /* total number of blocks */
    haiku_off_t     free_blocks;                        /* number of free blocks */
    haiku_off_t     total_nodes;                        /* total number of nodes */
    haiku_off_t     free_nodes;                         /* number of free nodes */
    char            device_name[128];                   /* device holding fs */
    char            volume_name[B_FILE_NAME_LENGTH];    /* volume name */
    char            fsh_name[B_OS_NAME_LENGTH];         /* name of fs handler */
} haiku_fs_info;

#endif /* _FS_INFO_H */