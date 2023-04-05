/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_STAT_H__
#define __HAIKU_STAT_H__

#include "BeDefs.h"
#include "haiku_time.h"

struct haiku_stat
{
    haiku_dev_t st_dev;             /* device ID that this file resides on */
    haiku_ino_t st_ino;             /* this file's serial inode ID */
    haiku_mode_t st_mode;           /* file mode (rwx for user, group, etc) */
    haiku_nlink_t st_nlink;         /* number of hard links to this file */
    haiku_uid_t st_uid;             /* user ID of the owner of this file */
    haiku_gid_t st_gid;             /* group ID of the owner of this file */
    haiku_off_t st_size;            /* size in bytes of this file */
    haiku_dev_t st_rdev;            /* device type (not used) */
    haiku_blksize_t st_blksize;     /* preferred block size for I/O */
    struct haiku_timespec st_atim;  /* last access time */
    struct haiku_timespec st_mtim;  /* last modification time */
    struct haiku_timespec st_ctim;  /* last change time, not creation time */
    struct haiku_timespec st_crtim; /* creation time */
    uint32 st_type;                 /* attribute/index type */
    haiku_blkcnt_t st_blocks;       /* number of blocks allocated for object */
};

/* source compatibility with old stat structure */
#define haiku_st_atime st_atim.tv_sec
#define haiku_st_mtime st_mtim.tv_sec
#define haiku_st_ctime st_ctim.tv_sec
#define haiku_st_crtime st_crtim.tv_sec

/* extended file types */
#define HAIKU_S_ATTR_DIR 01000000000         /* attribute directory */
#define HAIKU_S_ATTR 02000000000             /* attribute */
#define HAIKU_S_INDEX_DIR 04000000000        /* index (or index directory) */
#define HAIKU_S_STR_INDEX 00100000000        /* string index */
#define HAIKU_S_INT_INDEX 00200000000        /* int32 index */
#define HAIKU_S_UINT_INDEX 00400000000       /* uint32 index */
#define HAIKU_S_LONG_LONG_INDEX 00010000000  /* int64 index */
#define HAIKU_S_ULONG_LONG_INDEX 00020000000 /* uint64 index */
#define HAIKU_S_FLOAT_INDEX 00040000000      /* float index */
#define HAIKU_S_DOUBLE_INDEX 00001000000     /* double index */
#define HAIKU_S_ALLOW_DUPS 00002000000       /* allow duplicate entries (currently unused) */

/* link types */
#define HAIKU_S_LINK_SELF_HEALING 00001000000 /* link will be updated if you move its target */
#define HAIKU_S_LINK_AUTO_DELETE 00002000000  /* link will be deleted if you delete its target */

/* standard file types */
#define HAIKU_S_IFMT 00000170000   /* type of file */
#define HAIKU_S_IFSOCK 00000140000 /* socket */
#define HAIKU_S_IFLNK 00000120000  /* symbolic link */
#define HAIKU_S_IFREG 00000100000  /* regular */
#define HAIKU_S_IFBLK 00000060000  /* block special */
#define HAIKU_S_IFDIR 00000040000  /* directory */
#define HAIKU_S_IFCHR 00000020000  /* character special */
#define HAIKU_S_IFIFO 00000010000  /* fifo */

#define HAIKU_S_ISREG(mode) (((mode)&HAIKU_S_IFMT) == HAIKU_S_IFREG)
#define HAIKU_S_ISLNK(mode) (((mode)&HAIKU_S_IFMT) == HAIKU_S_IFLNK)
#define HAIKU_S_ISBLK(mode) (((mode)&HAIKU_S_IFMT) == HAIKU_S_IFBLK)
#define HAIKU_S_ISDIR(mode) (((mode)&HAIKU_S_IFMT) == HAIKU_S_IFDIR)
#define HAIKU_S_ISCHR(mode) (((mode)&HAIKU_S_IFMT) == HAIKU_S_IFCHR)
#define HAIKU_S_ISFIFO(mode) (((mode)&HAIKU_S_IFMT) == HAIKU_S_IFIFO)
#define HAIKU_S_ISSOCK(mode) (((mode)&HAIKU_S_IFMT) == HAIKU_S_IFSOCK)
#define HAIKU_S_ISINDEX(mode) (((mode)&HAIKU_S_INDEX_DIR) == HAIKU_S_INDEX_DIR)

#define HAIKU_S_IUMSK 07777 /* user settable bits */

#define HAIKU_S_ISUID 04000 /* set user id on execution */
#define HAIKU_S_ISGID 02000 /* set group id on execution */

#define HAIKU_S_ISVTX 01000 /* save swapped text even after use (sticky bit) */

#define HAIKU_S_IRWXU 00700 /* read, write, execute: owner */
#define HAIKU_S_IRUSR 00400 /* read permission: owner */
#define HAIKU_S_IWUSR 00200 /* write permission: owner */
#define HAIKU_S_IXUSR 00100 /* execute permission: owner */
#define HAIKU_S_IRWXG 00070 /* read, write, execute: group */
#define HAIKU_S_IRGRP 00040 /* read permission: group */
#define HAIKU_S_IWGRP 00020 /* write permission: group */
#define HAIKU_S_IXGRP 00010 /* execute permission: group */
#define HAIKU_S_IRWXO 00007 /* read, write, execute: other */
#define HAIKU_S_IROTH 00004 /* read permission: other */
#define HAIKU_S_IWOTH 00002 /* write permission: other */
#define HAIKU_S_IXOTH 00001 /* execute permission: other */

/* BSD extensions */
#define HAIKU_S_IREAD HAIKU_S_IRUSR
#define HAIKU_S_IWRITE HAIKU_S_IWUSR
#define HAIKU_S_IEXEC HAIKU_S_IXUSR

#define HAIKU_ACCESSPERMS (HAIKU_S_IRWXU | HAIKU_S_IRWXG | HAIKU_S_IRWXO)
#define HAIKU_ALLPERMS (HAIKU_S_ISUID | HAIKU_S_ISGID | HAIKU_S_ISVTX | HAIKU_S_IRWXU | HAIKU_S_IRWXG | HAIKU_S_IRWXO)
#define HAIKU_DEFFILEMODE (HAIKU_S_IRUSR | HAIKU_S_IWUSR | HAIKU_S_IRGRP | HAIKU_S_IWGRP | HAIKU_S_IROTH | HAIKU_S_IWOTH)
/* default file mode, everyone can read/write */

/* special values for timespec::tv_nsec passed to utimensat(), futimens() */
#define HAIKU_UTIME_NOW 1000000000
#define HAIKU_UTIME_OMIT 1000000001

enum
{
    B_STAT_MODE                 = 0x0001,
    B_STAT_UID                  = 0x0002,
    B_STAT_GID                  = 0x0004,
    B_STAT_SIZE                 = 0x0008,
    B_STAT_ACCESS_TIME          = 0x0010,
    B_STAT_MODIFICATION_TIME    = 0x0020,
    B_STAT_CREATION_TIME        = 0x0040,
    B_STAT_CHANGE_TIME          = 0x0080,

    B_STAT_INTERIM_UPDATE       = 0x1000
};

#endif /* __HAIKU_STAT_H__ */