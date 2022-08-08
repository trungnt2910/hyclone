#include <algorithm>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "errno_conversion.h"
#include "export.h"
#include "haiku_errors.h"
#include "haiku_socket.h"
#include "linux_debug.h"
#include "linux_syscall.h"

typedef uint32_t haiku_in_addr_t;

/* IP Version 4 address */
struct haiku_in_addr
{
    haiku_in_addr_t s_addr;
};

/* IP Version 4 socket address */
struct haiku_sockaddr_in
{
    uint8_t                 sin_len;
    uint8_t                 sin_family;
    uint16_t                sin_port;
    struct haiku_in_addr    sin_addr;
    int8_t                  sin_zero[24];
};

struct haiku_in6_addr
{
    union
    {
        uint8_t                 u6_addr8[16];
        uint16_t                u6_addr16[8];
        uint32_t                u6_addr32[4];
    } in6_u;
    #define haiku_s6_addr       in6_u.u6_addr8
    #define haiku_s6_addr16     in6_u.u6_addr16
    #define haiku_s6_addr32     in6_u.u6_addr32
};

/* IP Version 6 socket address. */
struct haiku_sockaddr_in6
{
    uint8_t                     sin6_len;
    uint8_t                     sin6_family;
    uint16_t                    sin6_port;
    uint32_t                    sin6_flowinfo;
    struct haiku_in6_addr       sin6_addr;
    uint32_t                    sin6_scope_id;
};

static int SocketFamilyBToLinux(int family);
static int SocketTypeBToLinux(int type);
static int SocketProtocolBToLinux(int protocol);
static int SocketAddressBToLinux(const struct haiku_sockaddr *addr, haiku_socklen_t addrlen,
                                  struct sockaddr_storage *storage);
static int SocketAddressLinuxToB(const struct sockaddr *addr, struct haiku_sockaddr_storage *storage);
static int SendMessageFlagsBToLinux(int flags);

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
