#include <errno.h>

#include "linux_debug.h"
#include "errno_conversion.h"
#include "haiku_errors.h"

int BToLinux(int bErrno)
{
    panic("BToLinux not implemented");
}

int LinuxToB(int linuxErrno)
{
    switch (linuxErrno)
    {
        case E2BIG:
            return HAIKU_POSIX_E2BIG;
        case EACCES:
            return HAIKU_POSIX_EACCES;
        case EADDRINUSE:
            return HAIKU_POSIX_EADDRINUSE;
        case EADDRNOTAVAIL:
            return HAIKU_POSIX_EADDRNOTAVAIL;
        case EAFNOSUPPORT:
            return HAIKU_POSIX_EAFNOSUPPORT;
        case EAGAIN:
            return HAIKU_POSIX_EAGAIN;
        case EALREADY:
            return HAIKU_POSIX_EALREADY;
        case EBADF:
            return HAIKU_POSIX_EBADF;
        case EBADMSG:
            return HAIKU_POSIX_EBADMSG;
        case EBUSY:
            return HAIKU_POSIX_EBUSY;
        case ECANCELED:
            return HAIKU_POSIX_ECANCELED;
        case ECHILD:
            return HAIKU_POSIX_ECHILD;
        case ECONNABORTED:
            return HAIKU_POSIX_ECONNABORTED;
        case ECONNREFUSED:
            return HAIKU_POSIX_ECONNREFUSED;
        case ECONNRESET:
            return HAIKU_POSIX_ECONNRESET;
        case EDEADLK:
            return HAIKU_POSIX_EDEADLK;
        case EDESTADDRREQ:
            return HAIKU_POSIX_EDESTADDRREQ;
        case EDOM:
            return HAIKU_POSIX_EDOM;
        case EDQUOT:
            return HAIKU_POSIX_EDQUOT;
        case EEXIST:
            return HAIKU_POSIX_EEXIST;
        case EFAULT:
            return HAIKU_POSIX_EFAULT;
        case EFBIG:
            return HAIKU_POSIX_EFBIG;
        case EHOSTUNREACH:
            return HAIKU_POSIX_EHOSTUNREACH;
        case EIDRM:
            return HAIKU_POSIX_EIDRM;
        case EILSEQ:
            return HAIKU_POSIX_EILSEQ;
        case EINPROGRESS:
            return HAIKU_POSIX_EINPROGRESS;
        case EINTR:
            return HAIKU_POSIX_EINTR;
        case EINVAL:
            return HAIKU_POSIX_EINVAL;
        case EIO:
            return HAIKU_POSIX_EIO;
        case EISCONN:
            return HAIKU_POSIX_EISCONN;
        case EISDIR:
            return HAIKU_POSIX_EISDIR;
        case ELOOP:
            return HAIKU_POSIX_ELOOP;
        case EMFILE:
            return HAIKU_POSIX_EMFILE;
        case EMLINK:
            return HAIKU_POSIX_EMLINK;
        case EMSGSIZE:
            return HAIKU_POSIX_EMSGSIZE;
        case ENAMETOOLONG:
            return HAIKU_POSIX_ENAMETOOLONG;
        case ENFILE:
            return HAIKU_POSIX_ENFILE;
        case ENOBUFS:
            return HAIKU_POSIX_ENOBUFS;
        case ENODEV:
            return HAIKU_POSIX_ENODEV;
        case ENOENT:
            return HAIKU_POSIX_ENOENT;
        case ENOEXEC:
            return HAIKU_POSIX_ENOEXEC;
        case ENOLCK:
            return HAIKU_POSIX_ENOLCK;
        case ENOMEM:
            return HAIKU_POSIX_ENOMEM;
        case ENOSPC:
            return HAIKU_POSIX_ENOSPC;
        case ENOSYS:
            return HAIKU_POSIX_ENOSYS;
        case ENOTCONN:
            return HAIKU_POSIX_ENOTCONN;
        case ENOTDIR:
            return HAIKU_POSIX_ENOTDIR;
        case ENOTEMPTY:
            return HAIKU_POSIX_ENOTEMPTY;
        case ENOTSOCK:
            return HAIKU_POSIX_ENOTSOCK;
        case ENOTSUP:
            return HAIKU_POSIX_ENOTSUP;
        case ENXIO:
            return HAIKU_POSIX_ENXIO;
        case EPERM:
            return HAIKU_POSIX_EPERM;
        case EPIPE:
            return HAIKU_POSIX_EPIPE;
        case EPROTO:
            return HAIKU_POSIX_EPROTO;
        case EPROTONOSUPPORT:
            return HAIKU_POSIX_EPROTONOSUPPORT;
        case EPROTOTYPE:
            return HAIKU_POSIX_EPROTOTYPE;
        case ERANGE:
            return HAIKU_POSIX_ERANGE;
        case EROFS:
            return HAIKU_POSIX_EROFS;
        case ESPIPE:
            return HAIKU_POSIX_ESPIPE;
        case ESRCH:
            return HAIKU_POSIX_ESRCH;
        case ESTALE:
            return HAIKU_POSIX_ESTALE;
        case ETIMEDOUT:
            return HAIKU_POSIX_ETIMEDOUT;
        case ETXTBSY:
            return HAIKU_POSIX_ETXTBSY;
        case EXDEV:
            return HAIKU_POSIX_EXDEV;
        default:
            // POSIX says these may or may not be the same value.
            if (linuxErrno == EOPNOTSUPP)
            {
                return HAIKU_POSIX_EOPNOTSUPP;
            }
            else if (linuxErrno == EWOULDBLOCK)
            {
                return HAIKU_POSIX_EWOULDBLOCK;
            }
            else if (linuxErrno == 0)
            {
                return 0;
            }
            return B_BAD_VALUE;
    }
}