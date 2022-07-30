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
