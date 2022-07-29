#include <cstdint>
#include <cstring>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "BeDefs.h"
#include "errno_conversion.h"
#include "export.h"
#include "extended_commpage.h"
#include "fcntl_conversion.h"
#include "haiku_errors.h"
#include "haiku_fcntl.h"
#include "haiku_signal.h"
#include "linux_debug.h"
#include "linux_syscall.h"
#include "linux_version.h"
#include "realpath.h"
#include "signal_conversion.h"
#include "stringutils.h"

/* access modes */
#define HAIKU_R_OK          4
#define HAIKU_W_OK          2
#define HAIKU_X_OK          1
#define HAIKU_F_OK          0

struct haiku_timespec
{
    time_t  tv_sec;        /* seconds */
    long    tv_nsec;    /* and nanoseconds */
};

// Be careful! Some of these types (dev_t, ino_t)
// may be different on Haiku and Linux.
struct haiku_stat
{
    haiku_dev_t              st_dev;            /* device ID that this file resides on */
    haiku_ino_t              st_ino;            /* this file's serial inode ID */
    haiku_mode_t             st_mode;           /* file mode (rwx for user, group, etc) */
    haiku_nlink_t            st_nlink;          /* number of hard links to this file */
    haiku_uid_t              st_uid;            /* user ID of the owner of this file */
    haiku_gid_t              st_gid;            /* group ID of the owner of this file */
    haiku_off_t              st_size;           /* size in bytes of this file */
    haiku_dev_t              st_rdev;           /* device type (not used) */
    haiku_blksize_t          st_blksize;        /* preferred block size for I/O */
    struct haiku_timespec    st_atim;           /* last access time */
    struct haiku_timespec    st_mtim;           /* last modification time */
    struct haiku_timespec    st_ctim;           /* last change time, not creation time */
    struct haiku_timespec    st_crtim;          /* creation time */
    uint32_t                 st_type;           /* attribute/index type */
    haiku_blkcnt_t           st_blocks;         /* number of blocks allocated for object */
};

typedef struct haiku_dirent
{
    haiku_dev_t             d_dev;      /* device */
    haiku_dev_t             d_pdev;     /* parent device (only for queries) */
    haiku_ino_t             d_ino;      /* inode number */
    haiku_ino_t             d_pino;     /* parent inode (only for queries) */
    unsigned short          d_reclen;   /* length of this record, not the name */
#if __GNUC__ == 2
    char                    d_name[0];  /* name of the entry (null byte terminated) */
#else
    char                    d_name[];   /* name of the entry (null byte terminated) */
#endif
} haiku_dirent_t;

typedef struct attr_info
{
    uint32_t    type;
    haiku_off_t size;
} attr_info;

struct linux_dirent
{
    unsigned long  d_ino;     /* Inode number */
    unsigned long  d_off;     /* Offset to next linux_dirent */
    unsigned short d_reclen;  /* Length of this linux_dirent */
    char           d_name[];  /* Filename (null-terminated) */
                              /* length is actually (d_reclen - 2 -
                                offsetof(struct linux_dirent, d_name)) */
    /*
    char           pad;       // Zero padding byte
    char           d_type;    // File type (only since Linux
                              // 2.6.4); offset is (d_reclen - 1)
    */
};

#define HAIKU_SEEK_SET 0
#define HAIKU_SEEK_CUR 1
#define HAIKU_SEEK_END 2

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

/* You can define your own FDSETSIZE if you need more bits - but
 * it should be enough for most uses.
 */
#ifndef HAIKU_FD_SETSIZE
#define HAIKU_FD_SETSIZE 1024
#endif

typedef uint32 haiku_fd_mask;

#ifndef _haiku_howmany
#define _haiku_howmany(x, y) (((x) + ((y) - 1)) / (y))
#endif

#ifndef haiku_howmany
#define haiku_howmany(x, y) (((x) + ((y) - 1)) / (y))
#endif

#define HAIKU_NFDBITS (sizeof(haiku_fd_mask) * 8) /* bits per mask */

typedef struct haiku_fd_set
{
    haiku_fd_mask bits[_haiku_howmany(HAIKU_FD_SETSIZE, HAIKU_NFDBITS)];
} haiku_fd_set;

#define _HAIKU_FD_BITSINDEX(fd) ((fd) / HAIKU_NFDBITS)
#define _HAIKU_FD_BIT(fd) (1L << ((fd) % HAIKU_NFDBITS))

/* FD_ZERO uses memset */
#include <string.h>

#define HAIKU_FD_ZERO(set) memset((set), 0, sizeof(haiku_fd_set))
#define HAIKU_FD_SET(fd, set) ((set)->bits[_HAIKU_FD_BITSINDEX(fd)] |= _HAIKU_FD_BIT(fd))
#define HAIKU_FD_CLR(fd, set) ((set)->bits[_HAIKU_FD_BITSINDEX(fd)] &= ~_HAIKU_FD_BIT(fd))
#define HAIKU_FD_ISSET(fd, set) ((set)->bits[_HAIKU_FD_BITSINDEX(fd)] & _HAIKU_FD_BIT(fd))
#define HAIKU_FD_COPY(source, target) (*(target) = *(source))

static int ModeBToLinux(int mode);
static int ModeLinuxToB(int mode);
static int SeekTypeBToLinux(int seekType);
static void FdSetBToLinux(const haiku_fd_set& fdSet, fd_set& linuxFdSet);
static bool IsTty(int fd);

extern "C"
{

// If pos is not -1, seek to pos.
// Then write bufferSize from buffer to fd.
// Returns errno if any error occurs.
ssize_t MONIKA_EXPORT _kern_write(int fd, haiku_off_t pos, const void* buffer, size_t bufferSize)
{
    // Haiku might use _kern_write to console with pos == 0.
    // However, Linux does not support seeking for console fds.
    if (pos != -1 && !(pos == 0 && IsTty(fd)))
    {
        off_t result = LINUX_SYSCALL3(__NR_lseek, fd, pos, SEEK_SET);
        if (result < 0)
        {
            return LinuxToB(-result);
        }
    }

    ssize_t bytesWritten = LINUX_SYSCALL3(__NR_write, fd, buffer, bufferSize);
    if (bytesWritten < 0)
    {
        return LinuxToB(-bytesWritten);
    }

    return bytesWritten;
}


ssize_t MONIKA_EXPORT _kern_read(int fd, haiku_off_t pos, void* buffer, size_t bufferSize)
{
    if (pos != -1 && !(pos == 0 && IsTty(fd)))
    {
        off_t result = LINUX_SYSCALL3(__NR_lseek, fd, pos, SEEK_SET);
        if (result < 0)
        {
            return LinuxToB(-result);
        }
    }

    ssize_t bytesRead = LINUX_SYSCALL3(__NR_read, fd, buffer, bufferSize);
    if (bytesRead < 0)
    {
        return LinuxToB(-bytesRead);
    }

    return bytesRead;
}

int MONIKA_EXPORT _kern_read_link(int fd, const char* path, char *buffer, size_t *_bufferSize)
{
    if (fd == HAIKU_AT_FDCWD)
    {
        fd = AT_FDCWD;
    }

    char hostPath[PATH_MAX];
    long status = GET_HOSTCALLS()->vchroot_expandat(fd, path, hostPath, sizeof(hostPath));

    if (status < 0)
    {
        return HAIKU_POSIX_ENOENT;
    }
    else if (status > sizeof(hostPath))
    {
        return HAIKU_POSIX_ENAMETOOLONG;
    }

    struct stat linuxStat;
    status = LINUX_SYSCALL2(__NR_lstat, hostPath, &linuxStat);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    status = LINUX_SYSCALL3(__NR_readlink, hostPath, buffer, *_bufferSize);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    *_bufferSize = linuxStat.st_size;

    return B_OK;
}

int MONIKA_EXPORT _kern_read_stat(int fd, const char* path, bool traverseLink, haiku_stat* st, size_t statSize)
{
    struct stat linuxstat;
    haiku_stat haikustat;
    long result;

    if (statSize > sizeof(haiku_stat))
    {
        return B_BAD_VALUE;
    }

    if (path != NULL)
    {
        char hostPath[PATH_MAX];
        if (GET_HOSTCALLS()->vchroot_expand(path, hostPath, sizeof(hostPath)) < 0)
        {
            return HAIKU_POSIX_EBADF;
        }

        if (traverseLink)
        {
            result = LINUX_SYSCALL2(__NR_stat, hostPath, &linuxstat);
        }
        else
        {
            result = LINUX_SYSCALL2(__NR_lstat, hostPath, &linuxstat);
        }
    }
    else
    {
        result = LINUX_SYSCALL2(__NR_fstat, fd, &linuxstat);
    }

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    haikustat.st_dev = linuxstat.st_dev;
    haikustat.st_ino = linuxstat.st_ino;
    haikustat.st_mode = ModeLinuxToB(linuxstat.st_mode);
    haikustat.st_nlink = linuxstat.st_nlink;
    haikustat.st_uid = linuxstat.st_uid;
    haikustat.st_gid = linuxstat.st_gid;
    haikustat.st_size = linuxstat.st_size;
    haikustat.st_rdev = linuxstat.st_rdev;
    haikustat.st_blksize = linuxstat.st_blksize;
    haikustat.st_atim.tv_sec = linuxstat.st_atim.tv_sec;
    haikustat.st_atim.tv_nsec = linuxstat.st_atim.tv_nsec;
    haikustat.st_mtim.tv_sec = linuxstat.st_mtim.tv_sec;
    haikustat.st_mtim.tv_nsec = linuxstat.st_mtim.tv_nsec;
    haikustat.st_ctim.tv_sec = linuxstat.st_ctim.tv_sec;
    haikustat.st_ctim.tv_nsec = linuxstat.st_ctim.tv_nsec;
    haikustat.st_blocks = linuxstat.st_blocks;
    // Unsupported fields.
    haikustat.st_crtim.tv_sec = 0;
    haikustat.st_crtim.tv_nsec = 0;
    haikustat.st_type = 0;

    memcpy(st, &haikustat, statSize);

    return B_OK;
}

int MONIKA_EXPORT _kern_write_stat(int fd, const char* path,
    bool traverseLink, const struct haiku_stat* stat,
    size_t statSize, int statMask)
{
    long result;

    if (statSize > sizeof(haiku_stat))
    {
        return B_BAD_VALUE;
    }

    if (fd >= 0)
    {
        if (statMask & B_STAT_MODE)
        {
            result = LINUX_SYSCALL3(__NR_fchmod, fd, ModeBToLinux(stat->st_mode), 0);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
        }
        if (statMask & B_STAT_UID)
        {
            result = LINUX_SYSCALL3(__NR_fchown, fd, stat->st_uid, -1);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
        }
        if (statMask & B_STAT_GID)
        {
            result = LINUX_SYSCALL3(__NR_fchown, fd, -1, stat->st_gid);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
        }
        if (statMask & B_STAT_SIZE)
        {
            result = LINUX_SYSCALL2(__NR_ftruncate, fd, stat->st_size);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
        }
        if (statMask & (B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME))
        {
            struct stat oldStat;
            result = LINUX_SYSCALL2(__NR_fstat, fd, &oldStat);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            struct timespec linuxTimespecs[2];
            linuxTimespecs[0].tv_sec = oldStat.st_atim.tv_sec;
            linuxTimespecs[0].tv_nsec = oldStat.st_atim.tv_nsec;
            linuxTimespecs[1].tv_sec = oldStat.st_mtim.tv_sec;
            linuxTimespecs[1].tv_nsec = oldStat.st_mtim.tv_nsec;
            if (statMask & B_STAT_ACCESS_TIME)
            {
                linuxTimespecs[0].tv_sec = stat->st_atim.tv_sec;
                linuxTimespecs[0].tv_nsec = stat->st_atim.tv_nsec;
            }
            if (statMask & B_STAT_MODIFICATION_TIME)
            {
                linuxTimespecs[1].tv_sec = stat->st_mtim.tv_sec;
                linuxTimespecs[1].tv_nsec = stat->st_mtim.tv_nsec;
            }
            result = LINUX_SYSCALL4(__NR_utimensat, fd, NULL, &linuxTimespecs, traverseLink ? 0 : AT_SYMLINK_NOFOLLOW);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
        }
    }
    else
    {
        char hostPath[PATH_MAX];
        if (GET_HOSTCALLS()->vchroot_expand(path, hostPath, sizeof(hostPath)) < 0)
        {
            return HAIKU_POSIX_EBADF;
        }

        if (statMask & B_STAT_MODE)
        {
            if (!traverseLink)
            {
                return HAIKU_POSIX_ENOSYS;
            }
            result = LINUX_SYSCALL3(__NR_chmod, hostPath, ModeBToLinux(stat->st_mode), 0);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
        }
        if (statMask & B_STAT_UID)
        {
            if (!traverseLink)
            {
                return HAIKU_POSIX_ENOSYS;
            }
            result = LINUX_SYSCALL3(__NR_chown, hostPath, stat->st_uid, -1);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
        }
        if (statMask & B_STAT_GID)
        {
            if (!traverseLink)
            {
                return HAIKU_POSIX_ENOSYS;
            }
            result = LINUX_SYSCALL3(__NR_chown, hostPath, -1, stat->st_gid);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
        }
        if (statMask & B_STAT_SIZE)
        {
            if (!traverseLink)
            {
                return HAIKU_POSIX_ENOSYS;
            }
            result = LINUX_SYSCALL2(__NR_truncate, hostPath, stat->st_size);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
        }
        if (statMask & (B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME))
        {
            struct timespec linuxTimespecs[2];
            linuxTimespecs[0].tv_sec = stat->st_atim.tv_sec;
            linuxTimespecs[0].tv_nsec = stat->st_atim.tv_nsec;
            linuxTimespecs[1].tv_sec = stat->st_mtim.tv_sec;
            linuxTimespecs[1].tv_nsec = stat->st_mtim.tv_nsec;
            if (statMask & B_STAT_ACCESS_TIME)
            {
                linuxTimespecs[0].tv_sec = stat->st_atim.tv_sec;
                linuxTimespecs[0].tv_nsec = stat->st_atim.tv_nsec;
            }
            if (statMask & B_STAT_MODIFICATION_TIME)
            {
                linuxTimespecs[1].tv_sec = stat->st_mtim.tv_sec;
                linuxTimespecs[1].tv_nsec = stat->st_mtim.tv_nsec;
            }
            result = LINUX_SYSCALL4(__NR_utimensat, AT_FDCWD, hostPath, &linuxTimespecs, traverseLink ? 0 : AT_SYMLINK_NOFOLLOW);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
        }
    }

    return B_OK;
}

int MONIKA_EXPORT _kern_open(int fd, const char* path, int openMode, int perms)
{
    if (fd == HAIKU_AT_FDCWD)
    {
        fd = AT_FDCWD;
    }

    bool noTraverse = (openMode & HAIKU_O_NOTRAVERSE);

    openMode &= ~HAIKU_O_NOTRAVERSE;
    int linuxFlags = OFlagsBToLinux(openMode);
    int linuxMode = ModeBToLinux(perms);

    char hostPath[PATH_MAX];
    if (GET_HOSTCALLS()->vchroot_expandat(fd, path, hostPath, sizeof(hostPath)) < 0)
    {
        return HAIKU_POSIX_EBADF;
    }

    if (noTraverse)
    {
        linuxFlags |= O_PATH | O_NOFOLLOW;
    }

    int result = LINUX_SYSCALL3(__NR_open, hostPath, linuxFlags, linuxMode);

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    if (linuxFlags & O_DIRECTORY)
    {
        GET_HOSTCALLS()->opendir(result);
    }

    return result;
}

int MONIKA_EXPORT _kern_close(int fd)
{
    int result = LINUX_SYSCALL1(__NR_close, fd);

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    GET_HOSTCALLS()->closedir(fd);

    return B_OK;
}

int MONIKA_EXPORT _kern_access(int fd, const char* path, int mode, bool effectiveUserGroup)
{
    if (fd == HAIKU_AT_FDCWD)
    {
        fd = AT_FDCWD;
    }

    int linuxMode = 0;

    if (mode == HAIKU_F_OK)
    {
        linuxMode = F_OK;
    }
    else
    {
        if (mode & HAIKU_R_OK)
        {
            linuxMode |= R_OK;
        }
        if (mode & HAIKU_W_OK)
        {
            linuxMode |= W_OK;
        }
        if (mode & HAIKU_X_OK)
        {
            linuxMode |= X_OK;
        }
    }

    long status;

    char hostPath[PATH_MAX];
    if (GET_HOSTCALLS()->vchroot_expand(path, hostPath, sizeof(hostPath)) < 0)
    {
        return HAIKU_POSIX_EBADF;
    }

    if (effectiveUserGroup)
    {
        if (!is_linux_version_at_least(5, 8))
        {
            return HAIKU_POSIX_ENOSYS;
        }
        status = LINUX_SYSCALL4(__NR_faccessat2, AT_FDCWD, hostPath, linuxMode, AT_EACCESS);
    }
    else
    {
        status = LINUX_SYSCALL3(__NR_faccessat, AT_FDCWD, hostPath, linuxMode);
    }

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

int MONIKA_EXPORT _kern_normalize_path(const char* userPath, bool traverseLink, char* buffer)
{
    if (!traverseLink)
    {
        panic("kern_normalize_path without traverseLink not implemented");
    }

    char hostPath[PATH_MAX];
    if (GET_HOSTCALLS()->vchroot_expand(userPath, hostPath, sizeof(hostPath)) < 0)
    {
        return HAIKU_POSIX_EBADF;
    }

    char resolvedPath[PATH_MAX];
    long result = realpath(hostPath, resolvedPath, traverseLink);

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    GET_HOSTCALLS()->vchroot_unexpand(resolvedPath, buffer, B_PATH_NAME_LENGTH);

    return B_OK;
}

int MONIKA_EXPORT _kern_create_pipe(int *fds)
{
    long result = LINUX_SYSCALL1(__NR_pipe, fds);

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    return B_OK;
}

int MONIKA_EXPORT _kern_rename(int oldDir, const char *oldpath, int newDir, const char *newpath)
{
    char oldHostPath[PATH_MAX];
    char newHostPath[PATH_MAX];

    long status = GET_HOSTCALLS()->vchroot_expandat(oldDir, oldpath, oldHostPath, sizeof(oldHostPath));
    if (status < 0)
    {
        return HAIKU_POSIX_EBADF;
    }
    else if (status > sizeof(oldHostPath))
    {
        return HAIKU_POSIX_ENAMETOOLONG;
    }

    status = GET_HOSTCALLS()->vchroot_expandat(newDir, newpath, newHostPath, sizeof(newHostPath));
    if (status < 0)
    {
        return HAIKU_POSIX_EBADF;
    }
    else if (status > sizeof(newHostPath))
    {
        return HAIKU_POSIX_ENAMETOOLONG;
    }

    status = LINUX_SYSCALL2(__NR_rename, oldHostPath, newHostPath);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

int MONIKA_EXPORT _kern_unlink(int fd, const char* path)
{
    if (fd == HAIKU_AT_FDCWD)
    {
        fd = AT_FDCWD;
    }

    char hostPath[PATH_MAX];
    if (GET_HOSTCALLS()->vchroot_expandat(fd, path, hostPath, sizeof(hostPath)) < 0)
    {
        return HAIKU_POSIX_EBADF;
    }

    int result = LINUX_SYSCALL3(__NR_unlinkat, AT_FDCWD, hostPath, 0);

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    return B_OK;
}

int MONIKA_EXPORT _kern_stat_attr(int fd, const char *attribute, struct attr_info *attrInfo)
{
    long result = LINUX_SYSCALL4(__NR_fgetxattr, fd, attribute, NULL, 0);
    if (result < 0)
    {
        return LinuxToB(-result);
    }
    attrInfo->size = result;
    attrInfo->type = 0;
    return B_OK;
}

int MONIKA_EXPORT _kern_open_dir(int fd, const char* path)
{
    if (fd == HAIKU_AT_FDCWD)
    {
        fd = AT_FDCWD;
    }

    char hostPath[PATH_MAX];
    if (GET_HOSTCALLS()->vchroot_expandat(fd, path, hostPath, sizeof(hostPath)) < 0)
    {
        return HAIKU_POSIX_EBADF;
    }

    int result = LINUX_SYSCALL2(__NR_open, hostPath, O_DIRECTORY);

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    // Registers the file descriptor as a directory.
    GET_HOSTCALLS()->opendir(result);

    return result;
}

ssize_t MONIKA_EXPORT _kern_read_dir(int fd, struct haiku_dirent *buffer, size_t bufferSize, uint32 maxCount)
{
    return GET_HOSTCALLS()->readdir(fd, buffer, bufferSize, maxCount);
}

haiku_off_t MONIKA_EXPORT _kern_seek(int fd, off_t pos, int seekType)
{
    if (fd == HAIKU_AT_FDCWD)
    {
        fd = AT_FDCWD;
    }

    int linuxSeekType = SeekTypeBToLinux(seekType);
    long result = LINUX_SYSCALL3(__NR_lseek, fd, pos, linuxSeekType);

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    return result;
}

status_t MONIKA_EXPORT _kern_getcwd(char *buffer, size_t size)
{
    char hostPath[PATH_MAX];
    long result = LINUX_SYSCALL2(__NR_getcwd, hostPath, sizeof(hostPath));

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    GET_HOSTCALLS()->vchroot_unexpand(hostPath, buffer, size);

    return B_OK;
}

int MONIKA_EXPORT _kern_dup(int fd)
{
    int result = LINUX_SYSCALL1(__NR_dup, fd);

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    return result;
}

int MONIKA_EXPORT _kern_dup2(int ofd, int nfd)
{
    int result = LINUX_SYSCALL2(__NR_dup2, ofd, nfd);

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    return result;
}

ssize_t MONIKA_EXPORT _kern_select(int numfds,
    struct haiku_fd_set *readSet, struct haiku_fd_set *writeSet, struct haiku_fd_set *errorSet,
    bigtime_t timeout, const haiku_sigset_t *sigMask)
{
    fd_set linuxReadSetMemory;
    fd_set linuxWriteSetMemory;
    fd_set linuxErrorSetMemory;

    fd_set *linuxReadSet = NULL;
    if (readSet != NULL)
    {
        FdSetBToLinux(*readSet, linuxReadSetMemory);
        linuxReadSet = &linuxReadSetMemory;
    }
    fd_set *linuxWriteSet = NULL;
    if (writeSet != NULL)
    {
        FdSetBToLinux(*writeSet, linuxWriteSetMemory);
        linuxWriteSet = &linuxWriteSetMemory;
    }
    fd_set *linuxErrorSet = NULL;
    if (errorSet != NULL)
    {
        FdSetBToLinux(*errorSet, linuxErrorSetMemory);
        linuxErrorSet = &linuxErrorSetMemory;
    }

    struct timespec linuxTimeout;
    linuxTimeout.tv_sec = timeout / 1000000;
    linuxTimeout.tv_nsec = (timeout % 1000000) * 1000;

    linux_sigset_t linuxSigMaskMemory;
    linux_sigset_t* linuxSigMask = NULL;

    struct linux_pselect_arg
    {
        const linux_sigset_t *ss;   /* Pointer to signal set */
        size_t ss_len;               /* Size (in bytes) of object
                                        pointed to by 'ss' */
    };

    if (sigMask != NULL)
    {
        linuxSigMaskMemory = SigSetBToLinux(*sigMask);
        linuxSigMask = &linuxSigMaskMemory;
    }

    struct linux_pselect_arg linuxSigMaskArg = { linuxSigMask, sizeof(linuxSigMaskMemory) };

    long result =
        LINUX_SYSCALL6(__NR_pselect6, numfds, linuxReadSet, linuxWriteSet, linuxErrorSet, &linuxTimeout, &linuxSigMaskArg);

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    return result;
}

status_t MONIKA_EXPORT _kern_setcwd(int fd, const char *path)
{
    if (fd == HAIKU_AT_FDCWD)
    {
        fd = AT_FDCWD;
    }

    char hostPath[PATH_MAX];
    if (GET_HOSTCALLS()->vchroot_expandat(fd, path, hostPath, sizeof(hostPath)) < 0)
    {
        return HAIKU_POSIX_ENOENT;
    }

    long result = LINUX_SYSCALL1(__NR_chdir, hostPath);

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    return B_OK;
}

status_t MONIKA_EXPORT _kern_create_dir(int fd, const char *path, int perms)
{
    char hostPath[PATH_MAX];
    long status = GET_HOSTCALLS()->vchroot_expandat(fd, path, hostPath, sizeof(hostPath));

    if (status < 0)
    {
        return HAIKU_POSIX_ENOENT;
    }
    else if (status > sizeof(hostPath))
    {
        return HAIKU_POSIX_ENAMETOOLONG;
    }

    status = LINUX_SYSCALL2(__NR_mkdir, hostPath, ModeBToLinux(perms));

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

status_t MONIKA_EXPORT _kern_read_fs_info(haiku_dev_t device, struct haiku_fs_info *info)
{
    return GET_SERVERCALLS()->read_fs_info(device, info);
}

// The functions below are clearly impossible
// to be cleanly implemented in Hyclone, so
// ENOSYS is directly returned and no logging is provided.
status_t MONIKA_EXPORT _kern_read_index_stat(dev_t device, const char *name, struct haiku_stat *stat)
{
    // No support for indexing.
    return HAIKU_POSIX_ENOSYS;
}

status_t MONIKA_EXPORT _kern_create_index(dev_t device, const char *name, uint32 type, uint32 flags)
{
    // No support for indexing.
    return HAIKU_POSIX_ENOSYS;
}

status_t MONIKA_EXPORT _kern_open_dir_entry_ref()
{
    //trace("stub: _kern_open_dir_entry_ref");
    return HAIKU_POSIX_ENOSYS;
}

status_t MONIKA_EXPORT _kern_open_entry_ref()
{
    //trace("stub: _kern_open_entry_ref");
    return HAIKU_POSIX_ENOSYS;
}

}

int ModeBToLinux(int mode)
{
    // Hopefully they're the same.
    return mode;
}

int ModeLinuxToB(int mode)
{
    // Hopefully they're the same.
    return mode;
}

int SeekTypeBToLinux(int seekType)
{
    switch (seekType)
    {
    case HAIKU_SEEK_SET:
        return SEEK_SET;
    case HAIKU_SEEK_CUR:
        return SEEK_CUR;
    case HAIKU_SEEK_END:
        return SEEK_END;
    default:
        panic("SeekTypeBToLinux: Unknown seek type.");
    }
}

void FdSetBToLinux(const struct haiku_fd_set& set, fd_set& linuxSet)
{
    FD_ZERO(&linuxSet);
    for (int i = 0; i < FD_SETSIZE; i++)
    {
        if (HAIKU_FD_ISSET(i, &set))
        {
            FD_SET(i, &linuxSet);
        }
    }
}

bool IsTty(int fd)
{
    termios termios;
    if (LINUX_SYSCALL3(__NR_ioctl, fd, TCGETS, &termios) < 0)
    {
        return false;
    }
    return true;
}