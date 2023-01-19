#include <errno.h>
#include <fcntl.h>

#include "errno_conversion.h"
#include "export.h"
#include "fcntl_conversion.h"
#include "haiku_errors.h"
#include "haiku_fcntl.h"
#include "linux_syscall.h"
#include "linux_debug.h"

static void FlockLinuxToB(const struct flock& linuxFlock, struct haiku_flock& haikuFlock);
static void FlockBToLinux(const struct haiku_flock& haikuFlock, struct flock& linuxFlock);
static short FlockLTypeLinuxToB(short type);
static short FlockLTypeBToLinux(short type);

extern "C"
{

int MONIKA_EXPORT _kern_fcntl(int fd, int op, size_t argument)
{
    long status = -EINVAL;
    switch (op)
    {
        case HAIKU_F_DUPFD:
            status = LINUX_SYSCALL3(__NR_fcntl, fd, F_DUPFD, argument);
            break;
        case HAIKU_F_GETFD:
            status = LINUX_SYSCALL2(__NR_fcntl, fd, F_GETFD);
            break;
        case HAIKU_F_SETFD:
            status = LINUX_SYSCALL3(__NR_fcntl, fd, F_SETFD, argument);
            break;
        case HAIKU_F_GETFL:
            status = LINUX_SYSCALL2(__NR_fcntl, fd, F_GETFL);
            if (status < 0)
            {
                return LinuxToB(-status);
            }
            else
            {
                return OFlagsLinuxToB(status);
            }
            break;
        case HAIKU_F_SETFL:
            status = LINUX_SYSCALL3(__NR_fcntl, fd, F_SETFL, OFlagsBToLinux(argument));
            break;
        case HAIKU_F_GETLK:
        {
            struct flock linux_flock;
            FlockBToLinux(*(struct haiku_flock*)argument, linux_flock);
            status = LINUX_SYSCALL3(__NR_fcntl, fd, F_GETLK, &linux_flock);
            FlockLinuxToB(linux_flock, *(struct haiku_flock*)argument);
            break;
        }
        case HAIKU_F_SETLK:
        {
            struct flock linux_flock;
            FlockBToLinux(*(struct haiku_flock*)argument, linux_flock);
            status = LINUX_SYSCALL3(__NR_fcntl, fd, F_SETLK, &linux_flock);
            break;
        }
        case HAIKU_F_SETLKW:
        {
            struct flock linux_flock;
            FlockBToLinux(*(struct haiku_flock*)argument, linux_flock);
            status = LINUX_SYSCALL3(__NR_fcntl, fd, F_SETLKW, &linux_flock);
            break;
        }
        case HAIKU_F_DUPFD_CLOEXEC:
            status = LINUX_SYSCALL3(__NR_fcntl, fd, F_DUPFD_CLOEXEC, argument);
            break;
        default:
            trace("Unsupported fcntl");
            return HAIKU_POSIX_EINVAL;
    }

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    // Status might contain some non-error information.
    return status;
}

}

void FlockLinuxToB(const struct flock& linuxFlock, struct haiku_flock& haikuFlock)
{
    haikuFlock.l_type = FlockLTypeLinuxToB(linuxFlock.l_type);
    haikuFlock.l_whence = SeekTypeLinuxToB(linuxFlock.l_whence);
    haikuFlock.l_start = linuxFlock.l_start;
    haikuFlock.l_len = linuxFlock.l_len;
    haikuFlock.l_pid = linuxFlock.l_pid;
}

void FlockBToLinux(const struct haiku_flock& haikuFlock, struct flock& linuxFlock)
{
    linuxFlock.l_type = FlockLTypeBToLinux(haikuFlock.l_type);
    linuxFlock.l_whence = SeekTypeBToLinux(haikuFlock.l_whence);
    linuxFlock.l_start = haikuFlock.l_start;
    linuxFlock.l_len = haikuFlock.l_len;
    linuxFlock.l_pid = haikuFlock.l_pid;
}

short FlockLTypeLinuxToB(short type)
{
    switch (type)
    {
        case F_RDLCK:
            return HAIKU_F_RDLCK;
        case F_WRLCK:
            return HAIKU_F_WRLCK;
        case F_UNLCK:
            return HAIKU_F_UNLCK;
        default:
            return -1;
    }
}

short FlockLTypeBToLinux(short type)
{
    switch (type)
    {
        case HAIKU_F_RDLCK:
            return F_RDLCK;
        case HAIKU_F_WRLCK:
            return F_WRLCK;
        case HAIKU_F_UNLCK:
            return F_UNLCK;
        default:
            return -1;
    }
}