#include <algorithm>
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "extended_commpage.h"
#include "haiku_netinet_in.h"
#include "linux_debug.h"
#include "socket_conversion.h"

int SocketFamilyBToLinux(int family)
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

int SocketTypeBToLinux(int type)
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

int SocketProtocolBToLinux(int protocol)
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

int SocketAddressBToLinux(const struct haiku_sockaddr *addr, haiku_socklen_t addrlen,
                                 struct sockaddr_storage *storage)
{
    switch (addr->sa_family)
    {
        case HAIKU_AF_UNIX:
        {
            struct sockaddr_un *un = (struct sockaddr_un *)storage;
            struct haiku_sockaddr_un *haiku_un = (struct haiku_sockaddr_un *)addr;
            char path[PATH_MAX], hostPath[PATH_MAX];
            size_t pathlen = addrlen - offsetof(struct haiku_sockaddr_un, sun_path);
            if (pathlen + 1 >= sizeof(path))
            {
                return -1;
            }
            memcpy(path, haiku_un->sun_path, pathlen);
            path[pathlen] = '\0';
            if (GET_HOSTCALLS()->vchroot_expand(path, hostPath, sizeof(hostPath)) >= sizeof(hostPath))
            {
                return -1;
            }
            un->sun_family = AF_UNIX;
            strncpy(un->sun_path, hostPath, std::min(sizeof(un->sun_path), sizeof(hostPath)));
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

int SocketAddressLinuxToB(const struct sockaddr *addr, struct haiku_sockaddr_storage *storage)
{
    switch (addr->sa_family)
    {
        case AF_UNIX:
        {
            struct sockaddr_un *linux_un = (struct sockaddr_un *)addr;
            struct haiku_sockaddr_un *haiku_un = (struct haiku_sockaddr_un *)storage;
            char path[PATH_MAX], hostPath[PATH_MAX];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overread"
            size_t pathlen = strnlen(linux_un->sun_path, sizeof(sockaddr_storage) - offsetof(struct sockaddr_un, sun_path));
#pragma GCC diagnostic pop
            if (pathlen + 1 >= sizeof(path))
            {
                return -1;
            }
            memcpy(hostPath, haiku_un->sun_path, pathlen);
            hostPath[pathlen] = '\0';
            if (GET_HOSTCALLS()->vchroot_unexpand(hostPath, path, sizeof(path)) >= sizeof(path))
            {
                return -1;
            }
            haiku_un->sun_family = HAIKU_AF_UNIX;
            strncpy(haiku_un->sun_path, path, std::min(sizeof(path), sizeof(haiku_un->sun_path)));
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

int SocketOptionBToLinux(int option)
{
    switch (option)
    {
#define SUPPORTED_SOCKET_OPTION(name) case HAIKU_##name: return name;
#define UNSUPPORTED_SOCKET_OPTION(name) case HAIKU_##name: trace("Unsupported socket option: "#name); return -1;
#include "socket_values.h"
#undef SUPPORTED_SOCKET_OPTION
#undef UNSUPPORTED_SOCKET_OPTION
        default:
            trace("Unsupported socket option.");
            return -1;
    }
}