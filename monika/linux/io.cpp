#include <cstdint>
#include <cstring>

#include <fcntl.h>
#include <poll.h>
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
#include "haiku_file.h"
#include "haiku_poll.h"
#include "haiku_signal.h"
#include "haiku_stat.h"
#include "haiku_uio.h"
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

#define CHECK_NULL_AND_RETURN_BAD_ADDRESS(str)                  \
    if (str == NULL)                                            \
    {                                                           \
        return B_BAD_ADDRESS;                                   \
    }

#define CHECK_NON_NULL_EMPTY_STRING_AND_RETURN(str, haikuError) \
    if (str != NULL && str[0] == '\0')                          \
    {                                                           \
        return haikuError;                                      \
    }

#define CHECK_FD_AND_PATH(fd, str)                              \
    if (fd == HAIKU_AT_FDCWD)                                   \
    {                                                           \
        CHECK_NON_NULL_EMPTY_STRING_AND_RETURN(str, B_ENTRY_NOT_FOUND);  \
        fd = AT_FDCWD;                                          \
    }                                                           \
    else                                                        \
    {                                                           \
        CHECK_NULL_AND_RETURN_BAD_ADDRESS(str);                 \
    }

static int ModeBToLinux(int mode);
static int ModeLinuxToB(int mode);
static void FdSetBToLinux(const haiku_fd_set& fdSet, fd_set& linuxFdSet);
static void FdSetLinuxToB(const fd_set& linuxFdSet, haiku_fd_set& fdSet);
static int PollEventsBToLinux(int pollEvents);
static int PollEventsLinuxToB(int pollEvents);
static int FlockFlagsBToLinux(int flockFlags);
static bool IsTty(int fd);

extern "C"
{

ssize_t MONIKA_EXPORT _kern_write(int fd, haiku_off_t pos, const void* buffer, size_t bufferSize)
{
    ssize_t bytesWritten;

    if (pos == -1)
    {
        bytesWritten = LINUX_SYSCALL3(__NR_write, fd, buffer, bufferSize);
    }
    else
    {
        bytesWritten = LINUX_SYSCALL4(__NR_pwrite64, fd, buffer, bufferSize, pos);
    }

    if (bytesWritten < 0)
    {
        return LinuxToB(-bytesWritten);
    }

    return bytesWritten;
}

ssize_t MONIKA_EXPORT _kern_writev(int fd, haiku_off_t pos, const struct haiku_iovec *vecs, size_t count)
{
    size_t memSize = ((count * sizeof(struct iovec)) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1);
    struct iovec *linuxVecs =
        (struct iovec *)
            LINUX_SYSCALL6(__NR_mmap, NULL, memSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if ((void*)linuxVecs == MAP_FAILED)
    {
        return HAIKU_POSIX_ENOMEM;
    }

    for (size_t i = 0; i < count; ++i)
    {
        linuxVecs[i].iov_base = vecs[i].iov_base;
        linuxVecs[i].iov_len = vecs[i].iov_len;
    }

    ssize_t bytesWritten;

    if (pos == -1)
    {
        bytesWritten = LINUX_SYSCALL3(__NR_writev, fd, linuxVecs, count);
    }
    else
    {
        // Last two params: Low and high order bytes of pos.
        bytesWritten = LINUX_SYSCALL5(__NR_pwritev, fd, linuxVecs, count, (uint32_t)pos, ((uint64_t)pos) >> 32);
    }

    LINUX_SYSCALL2(__NR_munmap, linuxVecs, memSize);

    if (bytesWritten < 0)
    {
        return LinuxToB(-bytesWritten);
    }

    return bytesWritten;
}

ssize_t MONIKA_EXPORT _kern_read(int fd, haiku_off_t pos, void* buffer, size_t bufferSize)
{
    ssize_t bytesRead;

    if (pos == -1)
    {
        bytesRead = LINUX_SYSCALL3(__NR_read, fd, buffer, bufferSize);
    }
    else
    {
        bytesRead = LINUX_SYSCALL4(__NR_pread64, fd, buffer, bufferSize, pos);
    }

    if (bytesRead < 0)
    {
        return LinuxToB(-bytesRead);
    }

    return bytesRead;
}

ssize_t MONIKA_EXPORT _kern_readv(int fd, off_t pos, const struct haiku_iovec *vecs, size_t count)
{
    size_t memSize = ((count * sizeof(struct iovec)) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1);
    struct iovec *linuxVecs =
        (struct iovec *)
            LINUX_SYSCALL6(__NR_mmap, NULL, memSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if ((void*)linuxVecs == MAP_FAILED)
    {
        return HAIKU_POSIX_ENOMEM;
    }

    for (size_t i = 0; i < count; ++i)
    {
        linuxVecs[i].iov_base = vecs[i].iov_base;
        linuxVecs[i].iov_len = vecs[i].iov_len;
    }

    ssize_t bytesRead;

    if (pos == -1)
    {
        bytesRead = LINUX_SYSCALL3(__NR_readv, fd, linuxVecs, count);
    }
    else
    {
        // Last two params: Low and high order bytes of pos.
        bytesRead = LINUX_SYSCALL5(__NR_preadv, fd, linuxVecs, count, (uint32_t)pos, ((uint64_t)pos) >> 32);
    }

    LINUX_SYSCALL2(__NR_munmap, linuxVecs, memSize);

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

status_t MONIKA_EXPORT _kern_create_symlink(int fd, const char *path, const char *toPath, int mode)
{
    // Linux does not support permissions on symlinks.
    (void)mode;

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

    // Absolute path symlink. We want to expand the target path.
    if (toPath[0] == '/')
    {
        char hostToPath[PATH_MAX];

        status = GET_HOSTCALLS()->vchroot_expand(toPath, hostToPath, sizeof(hostToPath));
        if (status < 0)
        {
            return HAIKU_POSIX_ENOENT;
        }
        else if (status > sizeof(hostToPath))
        {
            return HAIKU_POSIX_ENAMETOOLONG;
        }

        status = LINUX_SYSCALL2(__NR_symlink, hostToPath, hostPath);
    }
    else
    {
        status = LINUX_SYSCALL2(__NR_symlink, toPath, hostPath);
    }

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

status_t MONIKA_EXPORT _kern_create_link(int pathFD, const char *path, int toFD, const char *toPath, bool traverseLeafLink)
{
    if (pathFD == HAIKU_AT_FDCWD)
    {
        pathFD = AT_FDCWD;
    }

    char hostPath[PATH_MAX];
    long status = GET_HOSTCALLS()->vchroot_expandat(pathFD, path, hostPath, sizeof(hostPath));

    if (status < 0)
    {
        return HAIKU_POSIX_ENOENT;
    }
    else if (status > sizeof(hostPath))
    {
        return HAIKU_POSIX_ENAMETOOLONG;
    }

    if (toFD == HAIKU_AT_FDCWD)
    {
        toFD = AT_FDCWD;
    }

    char hostToPath[PATH_MAX];
    status = GET_HOSTCALLS()->vchroot_expandat(toFD, toPath, hostToPath, sizeof(hostToPath));

    if (status < 0)
    {
        return HAIKU_POSIX_ENOENT;
    }
    else if (status > sizeof(hostToPath))
    {
        return HAIKU_POSIX_ENAMETOOLONG;
    }


    status = LINUX_SYSCALL5(__NR_linkat, AT_FDCWD, hostToPath, AT_FDCWD, hostPath, traverseLeafLink ? AT_SYMLINK_FOLLOW : 0);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

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

    if (fd == HAIKU_AT_FDCWD)
    {
        fd = AT_FDCWD;
    }

    if (path != NULL)
    {
        char hostPath[PATH_MAX];

        if (traverseLink)
        {
            long expandStatus = GET_HOSTCALLS()->vchroot_expandlinkat(fd, path, hostPath, sizeof(hostPath));

            if (expandStatus < 0)
            {
                return HAIKU_POSIX_ENOENT;
            }
            else if (expandStatus > sizeof(hostPath))
            {
                return HAIKU_POSIX_ENAMETOOLONG;
            }

            result = LINUX_SYSCALL2(__NR_stat, hostPath, &linuxstat);
        }
        else
        {
            long expandStatus = GET_HOSTCALLS()->vchroot_expandat(fd, path, hostPath, sizeof(hostPath));

            if (expandStatus < 0)
            {
                return HAIKU_POSIX_ENOENT;
            }
            else if (expandStatus > sizeof(hostPath))
            {
                return HAIKU_POSIX_ENAMETOOLONG;
            }

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
    struct haiku_stat completeStat;

    if (stat == NULL)
    {
        return B_BAD_ADDRESS;
    }

    if (statSize > sizeof(haiku_stat))
    {
        return B_BAD_VALUE;
    }

    if (statSize < sizeof(haiku_stat))
    {
        memcpy(&completeStat, stat, statSize);
        stat = &completeStat;
    }

    if (fd != HAIKU_AT_FDCWD)
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
            struct timespec linuxTimespecs[2];
            linuxTimespecs[0].tv_nsec = UTIME_OMIT;
            linuxTimespecs[1].tv_nsec = UTIME_OMIT;
            if (statMask & B_STAT_ACCESS_TIME)
            {
                if (stat->st_atim.tv_nsec == HAIKU_UTIME_NOW)
                {
                    linuxTimespecs[0].tv_nsec = UTIME_NOW;
                }
                else
                {
                    linuxTimespecs[0].tv_sec = stat->st_atim.tv_sec;
                    linuxTimespecs[0].tv_nsec = stat->st_atim.tv_nsec;
                }
            }
            if (statMask & B_STAT_MODIFICATION_TIME)
            {
                if (stat->st_mtim.tv_nsec == HAIKU_UTIME_NOW)
                {
                    linuxTimespecs[1].tv_nsec = UTIME_NOW;
                }
                else
                {
                    linuxTimespecs[1].tv_sec = stat->st_mtim.tv_sec;
                    linuxTimespecs[1].tv_nsec = stat->st_mtim.tv_nsec;
                }
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
        CHECK_FD_AND_PATH(fd, path);

        char hostPath[PATH_MAX];
        long expandStatus;
        if (!traverseLink)
        {
            expandStatus = GET_HOSTCALLS()->vchroot_expandat(fd, path, hostPath, sizeof(hostPath));
        }
        else
        {
            expandStatus = GET_HOSTCALLS()->vchroot_expandlinkat(fd, path, hostPath, sizeof(hostPath));
        }

        if (expandStatus < 0)
        {
            return HAIKU_POSIX_ENOENT;
        }
        else if (expandStatus > sizeof(hostPath))
        {
            return HAIKU_POSIX_ENAMETOOLONG;
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
            linuxTimespecs[0].tv_nsec = UTIME_OMIT;
            linuxTimespecs[1].tv_nsec = UTIME_OMIT;
            if (statMask & B_STAT_ACCESS_TIME)
            {
                if (stat->st_atim.tv_nsec == HAIKU_UTIME_NOW)
                {
                    linuxTimespecs[0].tv_nsec = UTIME_NOW;
                }
                else
                {
                    linuxTimespecs[0].tv_sec = stat->st_atim.tv_sec;
                    linuxTimespecs[0].tv_nsec = stat->st_atim.tv_nsec;
                }
            }
            if (statMask & B_STAT_MODIFICATION_TIME)
            {
                if (stat->st_mtim.tv_nsec == HAIKU_UTIME_NOW)
                {
                    linuxTimespecs[1].tv_nsec = UTIME_NOW;
                }
                else
                {
                    linuxTimespecs[1].tv_sec = stat->st_mtim.tv_sec;
                    linuxTimespecs[1].tv_nsec = stat->st_mtim.tv_nsec;
                }
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
        CHECK_NON_NULL_EMPTY_STRING_AND_RETURN(path, HAIKU_POSIX_ENOENT);
        fd = AT_FDCWD;
    }
    bool noTraverse = (openMode & HAIKU_O_NOTRAVERSE);

    openMode &= ~HAIKU_O_NOTRAVERSE;
    int linuxFlags = OFlagsBToLinux(openMode);
    int linuxMode = ModeBToLinux(perms);

    char hostPath[PATH_MAX];

    long expandStatus;
    if (noTraverse)
    {
        expandStatus = GET_HOSTCALLS()->vchroot_expandat(fd, path, hostPath, sizeof(hostPath));
    }
    else
    {
        expandStatus = GET_HOSTCALLS()->vchroot_expandlinkat(fd, path, hostPath, sizeof(hostPath));
    }

    if (expandStatus < 0)
    {
        return HAIKU_POSIX_ENOENT;
    }
    else if (expandStatus > sizeof(hostPath))
    {
        return HAIKU_POSIX_ENAMETOOLONG;
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

    struct stat linuxStat;
    if (LINUX_SYSCALL2(__NR_fstat, result, &linuxStat) == 0 && S_ISDIR(linuxStat.st_mode))
    {
        GET_HOSTCALLS()->opendir(result);
        GET_SERVERCALLS()->register_entry_ref(linuxStat.st_dev, linuxStat.st_ino, hostPath, sizeof(hostPath));
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
    CHECK_FD_AND_PATH(fd, path);

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
    if (GET_HOSTCALLS()->vchroot_expandlinkat(fd, path, hostPath, sizeof(hostPath)) < 0)
    {
        return HAIKU_POSIX_EBADF;
    }

    if (effectiveUserGroup)
    {
        if (!is_linux_version_at_least(5, 8))
        {
            struct stat linuxstat;
            status = LINUX_SYSCALL4(__NR_newfstatat, AT_FDCWD, hostPath, &linuxstat, AT_SYMLINK_NOFOLLOW);

            if (status < 0)
            {
                return LinuxToB(-status);
            }

            if (linuxMode == F_OK)
            {
                return B_OK;
            }

            int euid = LINUX_SYSCALL0(__NR_geteuid);

            // root can read and write any file, and execute any file that anyone can execute.
            if (euid == 0 && ((linuxMode & X_OK) == 0 || (linuxstat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))))
            {
                return B_OK;
            }

            int granted = 0;

            if (euid == linuxstat.st_uid)
            {
                granted |= (linuxstat.st_mode >> 6) & linuxMode;
            }

            // TODO: Check group membership.
            if (linuxstat.st_gid == LINUX_SYSCALL0(__NR_getegid))
            {
                granted |= (linuxstat.st_mode >> 3) & linuxMode;
            }

            granted |= linuxstat.st_mode & linuxMode;

            if (granted == linuxMode)
            {
                return B_OK;
            }

            return HAIKU_POSIX_EACCES;
        }
        else
        {
            status = LINUX_SYSCALL4(__NR_faccessat2, AT_FDCWD, hostPath, linuxMode, AT_EACCESS);
        }
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

status_t MONIKA_EXPORT _kern_create_fifo(int fd, const char *path, mode_t perms)
{
    CHECK_FD_AND_PATH(fd, path);

    char hostPath[PATH_MAX];
    long status = GET_HOSTCALLS()->vchroot_expandat(fd, path, hostPath, sizeof(hostPath));
    if (status < 0)
    {
        return HAIKU_POSIX_EBADF;
    }
    else if (status > sizeof(hostPath))
    {
        return HAIKU_POSIX_ENAMETOOLONG;
    }

    status = LINUX_SYSCALL3(__NR_mknod, hostPath, perms | S_IFIFO, 0);
    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

int MONIKA_EXPORT _kern_rename(int oldDir, const char *oldpath, int newDir, const char *newpath)
{
    CHECK_FD_AND_PATH(oldDir, oldpath);
    CHECK_FD_AND_PATH(newDir, newpath);

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
    CHECK_FD_AND_PATH(fd, path);

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

int MONIKA_EXPORT _kern_open_attr_dir(int fd, const char *path, bool traverseLeafLink)
{
   trace("attribute directories are not implemented yet.");
   // This is returned on most Haiku filesystems.
   return B_UNSUPPORTED;
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
        CHECK_NON_NULL_EMPTY_STRING_AND_RETURN(path, HAIKU_POSIX_ENOENT);
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

    struct stat linuxStat;
    if (LINUX_SYSCALL2(__NR_fstat, result, &linuxStat) == 0)
    {
        GET_SERVERCALLS()->register_entry_ref(linuxStat.st_dev, linuxStat.st_ino, hostPath, sizeof(hostPath));
    }

    return result;
}

ssize_t MONIKA_EXPORT _kern_read_dir(int fd, struct haiku_dirent *buffer, size_t bufferSize, uint32 maxCount)
{
    return GET_HOSTCALLS()->readdir(fd, buffer, bufferSize, maxCount);
}

status_t MONIKA_EXPORT _kern_rewind_dir(int fd)
{
    GET_HOSTCALLS()->rewinddir(fd);
    return B_OK;
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

    if (readSet != NULL)
    {
        // Not a timeout.
        if (result > 0)
        {
            FdSetLinuxToB(linuxReadSetMemory, *readSet);
        }
        else
        {
            HAIKU_FD_ZERO(readSet);
        }
    }

    if (writeSet != NULL)
    {
        if (result > 0)
        {
            FdSetLinuxToB(linuxReadSetMemory, *writeSet);
        }
        else
        {
            HAIKU_FD_ZERO(writeSet);
        }
    }

    if (errorSet != NULL)
    {
        if (result > 0)
        {
            FdSetLinuxToB(linuxReadSetMemory, *errorSet);
        }
        else
        {
            HAIKU_FD_ZERO(errorSet);
        }
    }

    return result;
}

ssize_t MONIKA_EXPORT _kern_poll(struct haiku_pollfd *fds, int numFDs,
    bigtime_t timeout, const haiku_sigset_t *sigMask)
{
    size_t memSize = ((numFDs * sizeof(struct pollfd)) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1);
    struct pollfd *linuxFds =
        (struct pollfd *)
            LINUX_SYSCALL6(__NR_mmap, NULL, memSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    for (size_t i = 0; i < numFDs; ++i)
    {
        linuxFds[i].fd = fds[i].fd;
        linuxFds[i].events = PollEventsBToLinux(fds[i].events);
    }

    linux_sigset_t linuxSigMaskMemory;
    linux_sigset_t* linuxSigMask = NULL;

    if (sigMask != NULL)
    {
        linuxSigMaskMemory = SigSetBToLinux(*sigMask);
        linuxSigMask = &linuxSigMaskMemory;
    }

    struct timespec linuxTimeout;
    linuxTimeout.tv_sec = timeout / 1000000;
    linuxTimeout.tv_nsec = (timeout % 1000000) * 1000;

    long status = LINUX_SYSCALL5(__NR_ppoll, linuxFds, numFDs, &linuxTimeout, linuxSigMask, sizeof(linuxSigMaskMemory));

    if (status < 0)
    {
        LINUX_SYSCALL2(__NR_munmap, linuxFds, memSize);
        return LinuxToB(-status);
    }

    for (size_t i = 0; i < numFDs; ++i)
    {
        fds[i].revents = PollEventsLinuxToB(linuxFds[i].revents);
    }

    LINUX_SYSCALL2(__NR_munmap, linuxFds, memSize);
    return status;
}

status_t MONIKA_EXPORT _kern_setcwd(int fd, const char *path)
{
    // path can be null with a positive fd, this happens
    // in the implementation of fchdir.
    if (fd == HAIKU_AT_FDCWD)
    {
        fd = AT_FDCWD;
        CHECK_NULL_AND_RETURN_BAD_ADDRESS(path);
        CHECK_NON_NULL_EMPTY_STRING_AND_RETURN(path, B_ENTRY_NOT_FOUND);
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
    CHECK_FD_AND_PATH(fd, path);

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

status_t MONIKA_EXPORT _kern_remove_dir(int fd, const char *path)
{
    CHECK_FD_AND_PATH(fd, path);

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

    status = LINUX_SYSCALL1(__NR_rmdir, hostPath);

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

status_t MONIKA_EXPORT _kern_fsync(int fd)
{
    int status = LINUX_SYSCALL1(__NR_fsync, fd);
    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

status_t MONIKA_EXPORT _kern_flock(int fd, int op)
{
    int status = LINUX_SYSCALL2(__NR_flock, fd, FlockFlagsBToLinux(op));
    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

status_t MONIKA_EXPORT _kern_open_dir_entry_ref(dev_t device, ino_t inode, const char *name)
{
    char hostPath[PATH_MAX];
    long status = GET_SERVERCALLS()->get_entry_ref(device, inode, hostPath, sizeof(hostPath));
    if (status < 0)
    {
        if (status == B_BUFFER_OVERFLOW)
        {
            return B_NAME_TOO_LONG;
        }
        return status;
    }

    if (name)
    {
        size_t hostPathLen = status;
        if (hostPath[hostPathLen - 1] != '/')
        {
            hostPath[hostPathLen - 1] = '/';
        }
        else
        {
            --hostPathLen;
        }
        if (hostPathLen == sizeof(hostPath))
        {
            return B_NAME_TOO_LONG;
        }
        size_t nameLen = strlen(name);
        if (nameLen + hostPathLen >= sizeof(hostPath))
        {
            return B_NAME_TOO_LONG;
        }
        memcpy(hostPath + hostPathLen, name, nameLen + 1);
    }

    // TODO: Invoke realpath or vchroot or whatever to
    // get rid of potential symlinks that can break our current vchroot
    // mechanism.

    status = LINUX_SYSCALL2(__NR_open, hostPath, O_DIRECTORY);
    if (status < 0)
    {
        return LinuxToB(-status);
    }

    GET_HOSTCALLS()->opendir(status);

    return status;
}

status_t MONIKA_EXPORT _kern_open_entry_ref(dev_t device, ino_t inode, const char *name,
    int openMode, int perms)
{
    char hostPath[PATH_MAX];
    long status = GET_SERVERCALLS()->get_entry_ref(device, inode, hostPath, sizeof(hostPath));
    if (status < 0)
    {
        if (status == B_BUFFER_OVERFLOW)
        {
            return B_NAME_TOO_LONG;
        }
        return status;
    }

    if (name)
    {
        size_t hostPathLen = status;
        if (hostPath[hostPathLen - 1] != '/')
        {
            hostPath[hostPathLen - 1] = '/';
        }
        else
        {
            --hostPathLen;
        }
        if (hostPathLen == sizeof(hostPath))
        {
            return B_NAME_TOO_LONG;
        }
        size_t nameLen = strlen(name);
        if (nameLen + hostPathLen >= sizeof(hostPath))
        {
            return B_NAME_TOO_LONG;
        }
        memcpy(hostPath + hostPathLen, name, nameLen + 1);
    }

    bool noTraverse = (openMode & HAIKU_O_NOTRAVERSE);

    openMode &= ~HAIKU_O_NOTRAVERSE;
    int linuxFlags = OFlagsBToLinux(openMode);
    int linuxMode = ModeBToLinux(perms);

    if (noTraverse)
    {
        linuxFlags |= O_PATH | O_NOFOLLOW;
    }

    status = LINUX_SYSCALL3(__NR_open, hostPath, linuxFlags, linuxMode);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return status;
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

void FdSetBToLinux(const struct haiku_fd_set& set, fd_set& linuxSet)
{
    FD_ZERO(&linuxSet);
    for (int i = 0; i < FD_SETSIZE; ++i)
    {
        if (HAIKU_FD_ISSET(i, &set))
        {
            FD_SET(i, &linuxSet);
        }
    }
}

void FdSetLinuxToB(const fd_set& linuxSet, struct haiku_fd_set& set)
{
    HAIKU_FD_ZERO(&set);
    for (int i = 0; i < FD_SETSIZE; ++i)
    {
        if (FD_ISSET(i, &linuxSet))
        {
            HAIKU_FD_SET(i, &set);
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

int PollEventsBToLinux(int pollEvents)
{
    int linuxEvents = 0;
#define SUPPORTED_POLL_FLAG(name) if (pollEvents & HAIKU_##name) linuxEvents |= name;
#include "poll_values.h"
#undef SUPPORTED_POLL_FLAG
    return linuxEvents;
}

int PollEventsLinuxToB(int pollEvents)
{
    int haikuEvents = 0;
#define SUPPORTED_POLL_FLAG(name) if (pollEvents & name) haikuEvents |= HAIKU_##name;
#include "poll_values.h"
#undef SUPPORTED_POLL_FLAG
    return haikuEvents;
}

int FlockFlagsBToLinux(int flockFlags)
{
    int linuxFlags = 0;
    if (flockFlags & HAIKU_LOCK_SH)
    {
        linuxFlags |= LOCK_SH;
    }
    if (flockFlags & HAIKU_LOCK_EX)
    {
        linuxFlags |= LOCK_EX;
    }
    if (flockFlags & HAIKU_LOCK_NB)
    {
        linuxFlags |= LOCK_NB;
    }
    if (flockFlags & HAIKU_LOCK_UN)
    {
        linuxFlags |= LOCK_UN;
    }
    return linuxFlags;
}