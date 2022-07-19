/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_FCNTL_H__
#define __HAIKU_FCNTL_H__

#include "BeDefs.h"

/* commands that can be passed to fcntl() */
#define HAIKU_F_DUPFD         0x0001        /* duplicate fd */
#define HAIKU_F_GETFD         0x0002        /* get fd flags */
#define HAIKU_F_SETFD         0x0004        /* set fd flags */
#define HAIKU_F_GETFL         0x0008        /* get file status flags and access mode */
#define HAIKU_F_SETFL         0x0010        /* set file status flags */
#define HAIKU_F_GETLK         0x0020        /* get locking information */
#define HAIKU_F_SETLK         0x0080        /* set locking information */
#define HAIKU_F_SETLKW        0x0100        /* as above, but waits if blocked */
#define HAIKU_F_DUPFD_CLOEXEC 0x0200        /* duplicate fd with close on exec set */

/* advisory locking types */
#define HAIKU_F_RDLCK         0x0040        /* read or shared lock */
#define HAIKU_F_UNLCK         0x0200        /* unlock */
#define HAIKU_F_WRLCK         0x0400        /* write or exclusive lock */

/* file descriptor flags for fcntl() */
#define HAIKU_FD_CLOEXEC 1 /* close on exec */

/* file access modes for open() */
#define HAIKU_O_RDONLY        0x0000        /* read only */
#define HAIKU_O_WRONLY        0x0001        /* write only */
#define HAIKU_O_RDWR          0x0002        /* read and write */
#define HAIKU_O_ACCMODE       0x0003        /* mask to get the access modes above */
#define HAIKU_O_RWMASK        O_ACCMODE

/* flags for open() */
#define HAIKU_O_EXCL          0x0100        /* exclusive creat */
#define HAIKU_O_CREAT         0x0200        /* create and open file */
#define HAIKU_O_TRUNC         0x0400        /* open with truncation */
#define HAIKU_O_NOCTTY        0x1000        /* don't make tty the controlling tty */
#define HAIKU_O_NOTRAVERSE    0x2000        /* do not traverse leaf link */

/* flags for open() and fcntl() */
#define HAIKU_O_CLOEXEC       0x00000040    /* close on exec */
#define HAIKU_O_NONBLOCK      0x00000080    /* non blocking io */
#define HAIKU_O_NDELAY        O_NONBLOCK
#define HAIKU_O_APPEND        0x00000800    /* to end of file */
#define HAIKU_O_SYNC          0x00010000    /* write synchronized I/O file integrity */
#define HAIKU_O_RSYNC         0x00020000    /* read synchronized I/O file integrity */
#define HAIKU_O_DSYNC         0x00040000    /* write synchronized I/O data integrity */
#define HAIKU_O_NOFOLLOW      0x00080000    /* fail on symlinks */
#define HAIKU_O_DIRECT        0x00100000    /* do not use the file system cache if */
                                            /* possible */
#define HAIKU_O_NOCACHE       O_DIRECT
#define HAIKU_O_DIRECTORY     0x00200000    /* fail if not a directory */

/* flags for the *at() functions */
#define HAIKU_AT_FDCWD        (-1)          /* CWD FD for the *at() functions */

#define HAIKU_AT_SYMLINK_NOFOLLOW 0x01      /* fstatat(), fchmodat(), fchownat(), \
                                               utimensat() */
#define HAIKU_AT_SYMLINK_FOLLOW   0x02      /* linkat() */
#define HAIKU_AT_REMOVEDIR        0x04      /* unlinkat() */
#define HAIKU_AT_EACCESS          0x08      /* faccessat() */

/* file advisory information, unused by Haiku */
#define HAIKU_POSIX_FADV_NORMAL       0     /* no advice */
#define HAIKU_POSIX_FADV_SEQUENTIAL   1     /* expect sequential access */
#define HAIKU_POSIX_FADV_RANDOM       2     /* expect access in a random order */
#define HAIKU_POSIX_FADV_WILLNEED     3     /* expect access in the near future */
#define HAIKU_POSIX_FADV_DONTNEED     4     /* expect no access in the near future */
#define HAIKU_POSIX_FADV_NOREUSE      5     /* expect access only once */

/* advisory file locking */

struct haiku_flock
{
    short l_type;
    short l_whence;
    haiku_off_t l_start;
    haiku_off_t l_len;
    haiku_pid_t l_pid;
};

#endif /* _FCNTL_H */