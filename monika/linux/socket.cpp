#include <sys/socket.h>

#include "errno_conversion.h"
#include "export.h"
#include "haiku_socket.h"
#include "linux_debug.h"
#include "linux_syscall.h"

static int SocketFamilyBToLinux(int family);
static int SocketTypeBToLinux(int type);
static int SocketProtocolBToLinux(int protocol);

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
