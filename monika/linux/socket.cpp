#include <algorithm>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "errno_conversion.h"
#include "haiku_errors.h"
#include "haiku_netinet_in.h"
#include "haiku_netinet_tcp.h"
#include "haiku_socket.h"
#include "haiku_time.h"
#include "linux_debug.h"
#include "linux_syscall.h"
#include "socket_conversion.h"

static int SendMessageFlagsBToLinux(int flags);
static int TcpOptionBToLinux(int options);

extern "C"
{

int _moni_socket(int family, int type, int protocol)
{
    int linuxFamily = SocketFamilyBToLinux(family);
    int linuxType = SocketTypeBToLinux(type);
    int linuxProtocol = SocketProtocolBToLinux(protocol);

    if (linuxFamily < 0 || linuxType < 0 || linuxProtocol < 0)
    {
        return B_UNSUPPORTED;
    }

    long status = LINUX_SYSCALL3(__NR_socket, linuxFamily, linuxType, linuxProtocol);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return status;
}

status_t _moni_socketpair(int family, int type, int protocol, int *socketVector)
{
    int linuxFamily = SocketFamilyBToLinux(family);
    int linuxType = SocketTypeBToLinux(type);
    int linuxProtocol = SocketProtocolBToLinux(protocol);

    long status = LINUX_SYSCALL4(__NR_socketpair, linuxFamily, linuxType, linuxProtocol, socketVector);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

status_t _moni_bind(int socket, const struct haiku_sockaddr *address, haiku_socklen_t addressLength)
{
    struct sockaddr_storage linuxAddress;
    memset(&linuxAddress, 0, sizeof(linuxAddress));

    int length = SocketAddressBToLinux(address, addressLength, &linuxAddress);

    if (length < 0)
    {
        return HAIKU_POSIX_ENOSYS;
    }

    long status = LINUX_SYSCALL3(__NR_bind, socket, (struct sockaddr *)&linuxAddress, length);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

status_t _moni_shutdown_socket(int socket, int how)
{
    int linuxHow = 0;
    switch (how)
    {
        case HAIKU_SHUT_RD:
            linuxHow = SHUT_RD;
            break;
        case HAIKU_SHUT_WR:
            linuxHow = SHUT_WR;
            break;
        case HAIKU_SHUT_RDWR:
            linuxHow = SHUT_RDWR;
            break;
        default:
            return HAIKU_POSIX_EINVAL;
    }

    long status = LINUX_SYSCALL2(__NR_shutdown, socket, linuxHow);

    if (status < 0)
    {
        return LinuxToB(-status);
    }
}

status_t _moni_connect(int socket, const struct haiku_sockaddr *address, haiku_socklen_t addressLength)
{
    struct sockaddr_storage linuxAddress;
    memset(&linuxAddress, 0, sizeof(linuxAddress));

    int length = SocketAddressBToLinux(address, addressLength, &linuxAddress);

    if (length < 0)
    {
        return HAIKU_POSIX_ENOSYS;
    }

    long status = LINUX_SYSCALL3(__NR_connect, socket, (struct sockaddr *)&linuxAddress, length);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

status_t _moni_listen(int socket, int backlog)
{
    long status = LINUX_SYSCALL2(__NR_listen, socket, backlog);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

int _moni_accept(int socket, struct haiku_sockaddr *address,
    haiku_socklen_t *_addressLength)
{
    struct sockaddr_storage linuxAddressStorage;
    memset(&linuxAddressStorage, 0, sizeof(linuxAddressStorage));
    socklen_t linuxAddressLengthStorage = 0;

    struct sockaddr* linuxAddress = NULL;
    socklen_t* linuxAddressLength = NULL;

    if (address != NULL || _addressLength != NULL)
    {
        linuxAddress = (struct sockaddr*)&linuxAddressStorage;
        linuxAddressLength = &linuxAddressLengthStorage;
    }

    long fd = LINUX_SYSCALL3(__NR_accept, socket, linuxAddress, linuxAddressLength);

    if (fd < 0)
    {
        return LinuxToB(-fd);
    }

    if (address != NULL || _addressLength != NULL)
    {
        struct haiku_sockaddr_storage haikuAddressStorage;
        memset(&haikuAddressStorage, 0, sizeof(haikuAddressStorage));

        int realLength = SocketAddressLinuxToB(linuxAddress, &haikuAddressStorage);

        haiku_socklen_t memoryLength = (_addressLength == NULL) ? 0 : *_addressLength;
        haiku_socklen_t copyLength = std::min(memoryLength, (haiku_socklen_t)realLength);

        if (address != NULL)
        {
            memcpy(address, &haikuAddressStorage, copyLength);
        }

        if (_addressLength != NULL)
        {
            *_addressLength = copyLength;
        }
    }

    return fd;
}

ssize_t _moni_recvfrom(int socket, void *data, size_t length, int flags,
    struct haiku_sockaddr *address, haiku_socklen_t *_addressLength)
{
    struct sockaddr_storage linuxAddressStorage;
    memset(&linuxAddressStorage, 0, sizeof(linuxAddressStorage));
    socklen_t linuxAddressLengthStorage = 0;

    struct sockaddr* linuxAddress = NULL;
    socklen_t* linuxAddressLength = NULL;

    if (address != NULL || _addressLength != NULL)
    {
        linuxAddress = (struct sockaddr*)&linuxAddressStorage;
        linuxAddressLength = &linuxAddressLengthStorage;
        if (_addressLength != NULL)
        {
            *linuxAddressLength = *_addressLength;
        }
    }

    int linuxFlags = SendMessageFlagsBToLinux(flags);

    long bytesReceived = LINUX_SYSCALL6(__NR_recvfrom, socket, data, length,
        linuxFlags, linuxAddress, linuxAddressLength);

    if (bytesReceived < 0)
    {
        return LinuxToB(-bytesReceived);
    }

    if (address != NULL || _addressLength != NULL)
    {
        struct haiku_sockaddr_storage haikuAddressStorage;
        memset(&haikuAddressStorage, 0, sizeof(haikuAddressStorage));

        int realLength = SocketAddressLinuxToB(linuxAddress, &haikuAddressStorage);

        haiku_socklen_t memoryLength = (_addressLength == NULL) ? 0 : *_addressLength;
        haiku_socklen_t copyLength = std::min(memoryLength, (haiku_socklen_t)realLength);

        if (address != NULL)
        {
            memcpy(address, &haikuAddressStorage, copyLength);
        }

        if (_addressLength != NULL)
        {
            *_addressLength = copyLength;
        }
    }

    return bytesReceived;
}

ssize_t _moni_recv(int socket, void *data, size_t length, int flags)
{
    return _moni_recvfrom(socket, data, length, flags, NULL, NULL);
}

ssize_t _moni_sendto(int socket, const void *data, size_t length,
    int flags, const struct haiku_sockaddr *address, haiku_socklen_t addressLength)
{
    struct sockaddr_storage linuxAddressStorage;
    memset(&linuxAddressStorage, 0, sizeof(linuxAddressStorage));

    struct sockaddr* linuxAddress = NULL;
    socklen_t linuxAddressLength = 0;

    if (address != NULL)
    {
        linuxAddress = (struct sockaddr*)&linuxAddressStorage;
        linuxAddressLength = SocketAddressBToLinux(address, addressLength, &linuxAddressStorage);
    }

    int linuxFlags = SendMessageFlagsBToLinux(flags);

    long bytesSent = LINUX_SYSCALL6(__NR_sendto, socket, data, length,
        linuxFlags, linuxAddress, linuxAddressLength);

    if (bytesSent < 0)
    {
        return LinuxToB(-bytesSent);
    }

    return bytesSent;
}

ssize_t _moni_send(int socket, const void *data, size_t length, int flags)
{
    int linuxFlags = SendMessageFlagsBToLinux(flags);

    // According to man pages:
    // send(sockfd, buf, len, flags);
    //   is equivalent to
    // sendto(sockfd, buf, len, flags, NULL, 0);
    // LINUX_SYSCALLX macros automatically zeros out unused parameters for us.
    long status = LINUX_SYSCALL4(__NR_sendto, socket, data, length, linuxFlags);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return status;
}

status_t _moni_getsockopt(int socket, int level, int option,
    void *value, haiku_socklen_t *_length)
{
    // This function could, and SHOULD, be unified with _kern_setsockopt.
    int linuxLevel;
    socklen_t linuxLength = (_length) ? *_length : 0;

    switch (level)
    {
        case HAIKU_SOL_SOCKET:
        {
            linuxLevel = SOL_SOCKET;
            int linuxOption = SocketOptionBToLinux(option);

            switch (linuxOption)
            {
                // value is a int*
                case SO_ACCEPTCONN:
                case SO_BROADCAST:
                case SO_DEBUG:
                case SO_DONTROUTE:
                case SO_KEEPALIVE:
                case SO_OOBINLINE:
                case SO_REUSEADDR:
                case SO_REUSEPORT:
                case SO_SNDBUF:
                case SO_SNDLOWAT:
                case SO_RCVBUF:
                case SO_RCVLOWAT:
                // These two need marshalling.
                // case SO_TYPE:
                {
                    long status = LINUX_SYSCALL5(__NR_getsockopt, socket, linuxLevel, linuxOption, value, &linuxLength);
                    if (status < 0)
                    {
                        return LinuxToB(-status);
                    }
                    if (_length != NULL)
                    {
                        *_length = linuxLength;
                    }
                    return B_OK;
                }
                case SO_ERROR:
                {
                    int errValue = 0;
                    long status = LINUX_SYSCALL5(__NR_getsockopt, socket, linuxLevel, linuxOption, &errValue, &linuxLength);
                    if (status < 0)
                    {
                        return LinuxToB(-status);
                    }
                    errValue = LinuxToB(errValue);
                    memcpy(value, &errValue, sizeof(errValue));
                    if (_length != NULL)
                    {
                        *_length = linuxLength;
                    }
                    return B_OK;
                }
                case SO_SNDTIMEO:
                case SO_RCVTIMEO:
                {
                    if (_length == NULL || *_length < sizeof(struct haiku_timeval))
                    {
                        return B_BAD_VALUE;
                    }

                    struct timeval linuxTime;
                    linuxLength = sizeof(linuxTime);

                    long status = LINUX_SYSCALL5(__NR_getsockopt, socket, linuxLevel, linuxOption, &linuxTime, &linuxLength);
                    if (status < 0)
                    {
                        return LinuxToB(-status);
                    }

                    struct haiku_timeval haikuTime;
                    haikuTime.tv_sec = linuxTime.tv_sec;
                    haikuTime.tv_usec = linuxTime.tv_usec;
                    memcpy(value, &haikuTime, sizeof(haikuTime));

                    if (_length != NULL)
                    {
                        *_length = sizeof(haikuTime);
                    }
                    return B_OK;
                }
                default:
                    trace("_kern_getsockopt: Unimplemented SOL_SOCKET option.");
                    return HAIKU_POSIX_ENOSYS;
            }
        }
        break;
        case HAIKU_IPPROTO_TCP:
        {
            linuxLevel = IPPROTO_TCP;
            int linuxOption = TcpOptionBToLinux(option);

            switch (linuxOption)
            {
                case TCP_NODELAY:
                case TCP_MAXSEG:
                {
                    long status = LINUX_SYSCALL5(__NR_getsockopt, socket, linuxLevel, linuxOption, value, &linuxLength);
                    if (status < 0)
                    {
                        return LinuxToB(-status);
                    }
                    if (_length != NULL)
                    {
                        *_length = linuxLength;
                    }
                    return B_OK;
                }
                default:
                    trace("_kern_getsockopt: Unimplemented IPPROTO_TCP option.");
                    return HAIKU_POSIX_ENOSYS;
            }
        }
        break;
#define UNIMPLEMENTED_LEVEL(name) case HAIKU_##name: trace("Unimplemented getsockopt level: "#name); return HAIKU_POSIX_EPROTONOSUPPORT;
        UNIMPLEMENTED_LEVEL(IPPROTO_IP)
        UNIMPLEMENTED_LEVEL(IPPROTO_IPV6)
        UNIMPLEMENTED_LEVEL(IPPROTO_ICMP)
        UNIMPLEMENTED_LEVEL(IPPROTO_RAW)
        //UNIMPLEMENTED_LEVEL(IPPROTO_TCP)
        UNIMPLEMENTED_LEVEL(IPPROTO_UDP)
#undef UNIMPLEMENTED_LEVEL
        default:
            trace("Unsupported getsockopt level.");
            return HAIKU_POSIX_EPROTONOSUPPORT;
    }
}

status_t _moni_setsockopt(int socket, int level, int option,
    const void *value, haiku_socklen_t length)
{
    int linuxLevel;

    switch (level)
    {
        case HAIKU_SOL_SOCKET:
        {
            linuxLevel = SOL_SOCKET;
            int linuxOption = SocketOptionBToLinux(option);

            switch (linuxOption)
            {
                // value is a int*
                case SO_ACCEPTCONN:
                case SO_BROADCAST:
                case SO_DEBUG:
                case SO_DONTROUTE:
                case SO_KEEPALIVE:
                case SO_OOBINLINE:
                case SO_REUSEADDR:
                case SO_REUSEPORT:
                case SO_SNDBUF:
                case SO_SNDLOWAT:
                case SO_RCVBUF:
                case SO_RCVLOWAT:
                // These two need marshalling.
                // case SO_ERROR:
                // case SO_TYPE:
                {
                    long status = LINUX_SYSCALL5(__NR_setsockopt, socket, linuxLevel, linuxOption, value, length);
                    if (status < 0)
                    {
                        return LinuxToB(-status);
                    }
                    return B_OK;
                }
                case SO_SNDTIMEO:
                case SO_RCVTIMEO:
                {
                    if (length != sizeof(struct haiku_timeval))
                    {
                        return B_BAD_VALUE;
                    }

                    struct haiku_timeval* haikuTime = (struct haiku_timeval*)value;
                    struct timeval linuxTime;

                    // B_INFINITE_TIMEOUT would overflow and become a very small value
                    // on Linux.
                    if (haikuTime->tv_sec * 1000000 + haikuTime->tv_usec == B_INFINITE_TIMEOUT)
                    {
                        linuxTime.tv_sec = 0;
                        linuxTime.tv_usec = 0;
                    }
                    else
                    {
                        linuxTime.tv_sec = haikuTime->tv_sec;
                        linuxTime.tv_usec = haikuTime->tv_usec;
                    }

                    long status = LINUX_SYSCALL5(__NR_setsockopt, socket, linuxLevel, linuxOption, &linuxTime, length);

                    if (status < 0)
                    {
                        return LinuxToB(-status);
                    }
                    return B_OK;
                }
                default:
                    trace("_kern_setsockopt: Unimplemented SOL_SOCKET option.");
                    return HAIKU_POSIX_ENOSYS;
            }
        }
        break;
        case HAIKU_IPPROTO_TCP:
        {
            linuxLevel = IPPROTO_TCP;
            int linuxOption = TcpOptionBToLinux(option);

            switch (linuxOption)
            {
                case TCP_NODELAY:
                case TCP_MAXSEG:
                {
                    long status = LINUX_SYSCALL5(__NR_setsockopt, socket, linuxLevel, linuxOption, value, length);
                    if (status < 0)
                    {
                        return LinuxToB(-status);
                    }
                    return B_OK;
                }
                default:
                    trace("_kern_setsockopt: Unimplemented IPPROTO_TCP option.");
                    return HAIKU_POSIX_ENOSYS;
            }
        }
        break;
#define UNIMPLEMENTED_LEVEL(name) case HAIKU_##name: trace("Unimplemented setsockopt level: "#name); return HAIKU_POSIX_EPROTONOSUPPORT;
        UNIMPLEMENTED_LEVEL(IPPROTO_IP)
        UNIMPLEMENTED_LEVEL(IPPROTO_IPV6)
        UNIMPLEMENTED_LEVEL(IPPROTO_ICMP)
        UNIMPLEMENTED_LEVEL(IPPROTO_RAW)
        //UNIMPLEMENTED_LEVEL(IPPROTO_TCP)
        UNIMPLEMENTED_LEVEL(IPPROTO_UDP)
#undef UNIMPLEMENTED_LEVEL
        default:
            trace("Unsupported setsockopt level.");
            return HAIKU_POSIX_EPROTONOSUPPORT;
    }
}

status_t _moni_getpeername(int socket,
    struct haiku_sockaddr *address, haiku_socklen_t *_addressLength)
{
    struct sockaddr_storage linuxAddressStorage;
    struct sockaddr* linuxAddress = (struct sockaddr*)&linuxAddressStorage;
    socklen_t linuxAddressLength = sizeof(linuxAddressStorage);

    long status = LINUX_SYSCALL3(__NR_getpeername, socket, linuxAddress, &linuxAddressLength);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    struct haiku_sockaddr_storage haikuAddressStorage;
    haiku_socklen_t realLen = SocketAddressLinuxToB(linuxAddress, &haikuAddressStorage);
    haiku_socklen_t memoryLength = std::min(realLen, *_addressLength);

    memcpy(address, &haikuAddressStorage, memoryLength);

    *_addressLength = realLen;

    return B_OK;
}

status_t _moni_getsockname(int socket,
    struct sockaddr *address, socklen_t *_addressLength)
{
    struct sockaddr_storage linuxAddressStorage;
    struct sockaddr* linuxAddress = (struct sockaddr*)&linuxAddressStorage;
    socklen_t linuxAddressLength = sizeof(linuxAddressStorage);

    long status = LINUX_SYSCALL3(__NR_getsockname, socket, linuxAddress, &linuxAddressLength);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    struct haiku_sockaddr_storage haikuAddressStorage;
    haiku_socklen_t realLen = SocketAddressLinuxToB(linuxAddress, &haikuAddressStorage);
    haiku_socklen_t memoryLength = std::min(realLen, *_addressLength);

    memcpy(address, &haikuAddressStorage, memoryLength);

    *_addressLength = realLen;

    return B_OK;
}

}

static int SendMessageFlagsBToLinux(int flags)
{
    int linuxFlags = 0;
    if (flags & HAIKU_MSG_EOF)
    {
        linuxFlags |= MSG_FIN;
        flags &= ~HAIKU_MSG_EOF;
    }
#define SUPPORTED_SEND_MESSAGE_FLAG(name) if (flags & HAIKU_##name) linuxFlags |= name;
#define UNSUPPORTED_SEND_MESSAGE_FLAG(name) if (flags & HAIKU_##name) trace("Unsupported send message flag: "#name);
#include "socket_values.h"
#undef SUPPORTED_SEND_MESSAGE_FLAG
#undef UNSUPPORTED_SEND_MESSAGE_FLAG
    return linuxFlags;
}

static int TcpOptionBToLinux(int option)
{
    switch (option)
    {
#define SUPPORTED_TCP_OPTION(name) case HAIKU_##name: return name;
#define UNSUPPORTED_TCP_OPTION(name) case HAIKU_##name: trace("Unsupported tcp option: "#name); return -1;
#include "socket_values.h"
#undef SUPPORTED_TCP_OPTION
#undef UNSUPPORTED_TCP_OPTION
        default:
            trace("Unsupported tcp option.");
            return -1;
    }
}
