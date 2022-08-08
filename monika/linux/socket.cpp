#include <algorithm>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "errno_conversion.h"
#include "export.h"
#include "haiku_errors.h"
#include "haiku_netinet_in.h"
#include "haiku_netinet_tcp.h"
#include "haiku_socket.h"
#include "linux_debug.h"
#include "linux_syscall.h"

#include "extended_commpage.h"

static int SocketFamilyBToLinux(int family);
static int SocketTypeBToLinux(int type);
static int SocketProtocolBToLinux(int protocol);
static int SocketOptionBToLinux(int option);
static int SocketAddressBToLinux(const struct haiku_sockaddr *addr, haiku_socklen_t addrlen,
                                  struct sockaddr_storage *storage);
static int SocketAddressLinuxToB(const struct sockaddr *addr, struct haiku_sockaddr_storage *storage);
static int SendMessageFlagsBToLinux(int flags);
static int TcpOptionBToLinux(int options);

extern "C"
{

int MONIKA_EXPORT _kern_socket(int family, int type, int protocol)
{
    int linuxFamily = SocketFamilyBToLinux(family);
    int linuxType = SocketTypeBToLinux(type);
    int linuxProtocol = SocketProtocolBToLinux(protocol);

    long status = LINUX_SYSCALL3(__NR_socket, linuxFamily, linuxType, linuxProtocol);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return status;
}

status_t MONIKA_EXPORT _kern_socketpair(int family, int type, int protocol, int *socketVector)
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

status_t MONIKA_EXPORT _kern_connect(int socket, const struct haiku_sockaddr *address, haiku_socklen_t addressLength)
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

ssize_t MONIKA_EXPORT _kern_recvfrom(int socket, void *data, size_t length, int flags,
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

    long status = LINUX_SYSCALL6(__NR_recvfrom, socket, data, length,
        linuxFlags, linuxAddress, linuxAddressLength);

    if (status < 0)
    {
        return LinuxToB(-status);
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

    return status;
}

ssize_t MONIKA_EXPORT _kern_send(int socket, const void *data, size_t length, int flags)
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

status_t MONIKA_EXPORT _kern_setsockopt(int socket, int level, int option,
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
                default:
                    GET_HOSTCALLS()->printf("_kern_setsockopt: unsupported socket option %d\n", option);
                    trace("Unimplemented SOL_SOCKET option.");
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
                    GET_HOSTCALLS()->printf("_kern_setsockopt: unsupported TCP option %d\n", option);
                    trace("Unimplemented IPPROTO_TCP option.");
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

}

static int SocketFamilyBToLinux(int family)
{
    switch (family)
    {
#define SUPPORTED_SOCKET_FAMILY(name) case HAIKU_##name: return name;
#define UNSUPPORTED_SOCKET_FAMILY(name) case HAIKU_##name: trace("Unsupported socket family: "#name); return -1;
#include "socket_values.h"
#undef SUPPORTED_SOCKET_FAMILY
#undef UNSUPPORTED_SOCKET_FAMILY
        default:
            trace("Unsupported socket family.");
            return -1;
    }
}

static int SocketTypeBToLinux(int type)
{
    switch (type)
    {
#define SUPPORTED_SOCKET_TYPE(name) case HAIKU_##name: return name;
#include "socket_values.h"
#undef SUPPORTED_SOCKET_TYPE
        default:
            trace("Unsupported socket type.");
            return -1;
    }
}

static int SocketProtocolBToLinux(int protocol)
{
    if (protocol == 0)
    {
        return 0;
    }
    switch (protocol)
    {
#define SUPPORTED_SOCKET_PROTOCOL(name) case HAIKU_##name: return name;
#define UNSUPPORTED_SOCKET_PROTOCOL(name) case HAIKU_##name: trace("Unsupported socket protocol: "#name); return -1;
#include "socket_values.h"
#undef SUPPORTED_SOCKET_PROTOCOL
#undef UNSUPPORTED_SOCKET_PROTOCOL
        default:
            trace("Unsupported socket protocol.");
            return -1;
    }
}

static int SocketAddressBToLinux(const struct haiku_sockaddr *addr, haiku_socklen_t addrlen,
                                 struct sockaddr_storage *storage)
{
    switch (addr->sa_family)
    {
        case HAIKU_AF_UNIX:
        {
            struct sockaddr_un *un = (struct sockaddr_un *)storage;
            struct haiku_sockaddr_un *haiku_un = (struct haiku_sockaddr_un *)addr;
            un->sun_family = AF_UNIX;
            strncpy(un->sun_path, haiku_un->sun_path, std::min(sizeof(un->sun_path), sizeof(haiku_un->sun_path)));
            return sizeof(struct sockaddr_un);
        }
        case HAIKU_AF_INET:
        {
            struct sockaddr_in *in = (struct sockaddr_in *)storage;
            struct haiku_sockaddr_in *haiku_in = (struct haiku_sockaddr_in *)addr;
            in->sin_family = AF_INET;
            in->sin_port = haiku_in->sin_port;
            in->sin_addr.s_addr = haiku_in->sin_addr.s_addr;
            return sizeof(struct sockaddr_in);
        }
        case HAIKU_AF_INET6:
        {
            struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)storage;
            struct haiku_sockaddr_in6 *haiku_in6 = (struct haiku_sockaddr_in6 *)addr;
            in6->sin6_family = AF_INET6;
            in6->sin6_port = haiku_in6->sin6_port;
            in6->sin6_flowinfo = haiku_in6->sin6_flowinfo;
            memcpy(&in6->sin6_addr, &haiku_in6->sin6_addr, sizeof(in6->sin6_addr));
            in6->sin6_scope_id = haiku_in6->sin6_scope_id;
            return sizeof(struct sockaddr_in6);
        }
#define UNIMPLEMENTED_SOCKADDR(name) case HAIKU_##name: trace("Unimplemented sockaddr: "#name); return -1;
        //UNIMPLEMENTED_SOCKADDR(AF_INET);
        UNIMPLEMENTED_SOCKADDR(AF_APPLETALK);
        UNIMPLEMENTED_SOCKADDR(AF_ROUTE);
        UNIMPLEMENTED_SOCKADDR(AF_LINK);
        //UNIMPLEMENTED_SOCKADDR(AF_INET6);
        UNIMPLEMENTED_SOCKADDR(AF_DLI);
        UNIMPLEMENTED_SOCKADDR(AF_IPX);
        UNIMPLEMENTED_SOCKADDR(AF_NOTIFY);
        //UNIMPLEMENTED_SOCKADDR(AF_UNIX);
        UNIMPLEMENTED_SOCKADDR(AF_BLUETOOTH);
#undef UNIMPLEMENTED_SOCKADDR
        default:
            trace("Unsupported socket family.");
            return -1;
    }
}

static int SocketAddressLinuxToB(const struct sockaddr *addr, struct haiku_sockaddr_storage *storage)
{
    switch (addr->sa_family)
    {
        case AF_UNIX:
        {
            struct sockaddr_un *linux_un = (struct sockaddr_un *)addr;
            struct haiku_sockaddr_un *haiku_un = (struct haiku_sockaddr_un *)storage;
            haiku_un->sun_family = HAIKU_AF_UNIX;
            strncpy(haiku_un->sun_path, linux_un->sun_path, std::min(sizeof(linux_un->sun_path), sizeof(haiku_un->sun_path)));
            return sizeof(struct haiku_sockaddr_un);
        }
        case AF_INET:
        {
            struct sockaddr_in *linux_in = (struct sockaddr_in *)addr;
            struct haiku_sockaddr_in *haiku_in = (struct haiku_sockaddr_in *)storage;
            haiku_in->sin_family = HAIKU_AF_INET;
            haiku_in->sin_port = linux_in->sin_port;
            haiku_in->sin_addr.s_addr = linux_in->sin_addr.s_addr;
            return sizeof(struct haiku_sockaddr_in);
        }
        case AF_INET6:
        {
            struct sockaddr_in6 *linux_in6 = (struct sockaddr_in6 *)addr;
            struct haiku_sockaddr_in6 *haiku_in6 = (struct haiku_sockaddr_in6 *)storage;
            haiku_in6->sin6_family = HAIKU_AF_INET6;
            haiku_in6->sin6_port = linux_in6->sin6_port;
            haiku_in6->sin6_flowinfo = linux_in6->sin6_flowinfo;
            memcpy(&haiku_in6->sin6_addr, &linux_in6->sin6_addr, sizeof(linux_in6->sin6_addr));
            haiku_in6->sin6_scope_id = linux_in6->sin6_scope_id;
            return sizeof(struct haiku_sockaddr_in6);
        }
#define UNIMPLEMENTED_SOCKADDR(name) case ##name: trace("Unimplemented sockaddr: "#name); return -1;
        //UNIMPLEMENTED_SOCKADDR(AF_INET);
        UNIMPLEMENTED_SOCKADDR(AF_APPLETALK);
        UNIMPLEMENTED_SOCKADDR(AF_ROUTE);
        UNIMPLEMENTED_SOCKADDR(AF_LINK);
        //UNIMPLEMENTED_SOCKADDR(AF_INET6);
        UNIMPLEMENTED_SOCKADDR(AF_DLI);
        UNIMPLEMENTED_SOCKADDR(AF_IPX);
        UNIMPLEMENTED_SOCKADDR(AF_NOTIFY);
        //UNIMPLEMENTED_SOCKADDR(AF_UNIX);
        UNIMPLEMENTED_SOCKADDR(AF_BLUETOOTH);
#undef UNIMPLEMENTED_SOCKADDR
        default:
            trace("Unsupported socket family.");
            return -1;
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

static int SocketOptionBToLinux(int option)
{
    switch (option)
    {
#define SUPPORTED_SOCKET_OPTION(name) case HAIKU_##name: return name;
#define UNSUPPORTED_SOCKET_OPTION(name) case HAIKU_##name: trace("Unsupported socket option: "#name); return -1;
#include "socket_values.h"
#undef SUPPORTED_SOCKET_OPTION
#undef UNSUPPORTED_SOCKET_OPTION
        default:
            GET_HOSTCALLS()->printf("_kern_setsockopt: unsupported socket option %d\n", option);
            trace("Unsupported socket option.");
            return -1;
    }
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
            GET_HOSTCALLS()->printf("_kern_setsockopt: unsupported tcp option %d\n", option);
            trace("Unsupported tcp option.");
            return -1;
    }
}
