#ifndef SUPPORTED_SOCKET_FAMILY
#define SUPPORTED_SOCKET_FAMILY(name)
#define SUPPORTED_SOCKET_FAMILY_NOT_PREDEFINED
#endif

#ifndef UNSUPPORTED_SOCKET_FAMILY
#define UNSUPPORTED_SOCKET_FAMILY(name)
#define UNSUPPORTED_SOCKET_FAMILY_NOT_PREDEFINED
#endif

SUPPORTED_SOCKET_FAMILY(AF_UNSPEC)
SUPPORTED_SOCKET_FAMILY(AF_INET)
SUPPORTED_SOCKET_FAMILY(AF_APPLETALK)
SUPPORTED_SOCKET_FAMILY(AF_ROUTE)
SUPPORTED_SOCKET_FAMILY(AF_INET6)
SUPPORTED_SOCKET_FAMILY(AF_IPX)
//Synonym for AF_UNIX
//SUPPORTED_SOCKET_FAMILY(AF_LOCAL)
SUPPORTED_SOCKET_FAMILY(AF_UNIX)
SUPPORTED_SOCKET_FAMILY(AF_BLUETOOTH)

UNSUPPORTED_SOCKET_FAMILY(AF_LINK)
UNSUPPORTED_SOCKET_FAMILY(AF_DLI)
UNSUPPORTED_SOCKET_FAMILY(AF_NOTIFY)

#ifdef SUPPORTED_SOCKET_FAMILY_NOT_PREDEFINED
#undef SUPPORTED_SOCKET_FAMILY_NOT_PREDEFINED
#undef SUPPORTED_SOCKET_FAMILY
#endif

#ifdef UNSUPPORTED_SOCKET_FAMILY_NOT_PREDEFINED
#undef UNSUPPORTED_SOCKET_FAMILY_NOT_PREDEFINED
#undef UNSUPPORTED_SOCKET_FAMILY
#endif

#ifndef SUPPORTED_SOCKET_TYPE
#define SUPPORTED_SOCKET_TYPE(name)
#define SUPPORTED_SOCKET_TYPE_NOT_PREDEFINED
#endif

SUPPORTED_SOCKET_TYPE(SOCK_STREAM)
SUPPORTED_SOCKET_TYPE(SOCK_DGRAM)
SUPPORTED_SOCKET_TYPE(SOCK_RAW)
SUPPORTED_SOCKET_TYPE(SOCK_SEQPACKET)

#ifdef SUPPORTED_SOCKET_TYPE_NOT_PREDEFINED
#undef SUPPORTED_SOCKET_TYPE_NOT_PREDEFINED
#undef SUPPORTED_SOCKET_TYPE
#endif

#ifndef SUPPORTED_SOCKET_OPTION
#define SUPPORTED_SOCKET_OPTION(name)
#define SUPPORTED_SOCKET_OPTION_NOT_PREDEFINED
#endif

#ifndef UNSUPPORTED_SOCKET_OPTION
#define UNSUPPORTED_SOCKET_OPTION(name)
#define UNSUPPORTED_SOCKET_OPTION_NOT_PREDEFINED
#endif

SUPPORTED_SOCKET_OPTION(SO_ACCEPTCONN)
SUPPORTED_SOCKET_OPTION(SO_BROADCAST)
SUPPORTED_SOCKET_OPTION(SO_DEBUG)
SUPPORTED_SOCKET_OPTION(SO_DONTROUTE)
SUPPORTED_SOCKET_OPTION(SO_KEEPALIVE)
SUPPORTED_SOCKET_OPTION(SO_OOBINLINE)
SUPPORTED_SOCKET_OPTION(SO_REUSEADDR)
SUPPORTED_SOCKET_OPTION(SO_REUSEPORT)
SUPPORTED_SOCKET_OPTION(SO_LINGER)
SUPPORTED_SOCKET_OPTION(SO_SNDBUF)
SUPPORTED_SOCKET_OPTION(SO_SNDLOWAT)
SUPPORTED_SOCKET_OPTION(SO_SNDTIMEO)
SUPPORTED_SOCKET_OPTION(SO_RCVBUF)
SUPPORTED_SOCKET_OPTION(SO_RCVLOWAT)
SUPPORTED_SOCKET_OPTION(SO_RCVTIMEO)
SUPPORTED_SOCKET_OPTION(SO_ERROR)
SUPPORTED_SOCKET_OPTION(SO_TYPE)
SUPPORTED_SOCKET_OPTION(SO_BINDTODEVICE)
SUPPORTED_SOCKET_OPTION(SO_PEERCRED)

UNSUPPORTED_SOCKET_OPTION(SO_USELOOPBACK)
UNSUPPORTED_SOCKET_OPTION(SO_NONBLOCK)

#ifdef SUPPORTED_SOCKET_OPTION_NOT_PREDEFINED
#undef SUPPORTED_SOCKET_OPTION_NOT_PREDEFINED
#undef SUPPORTED_SOCKET_OPTION
#endif

#ifdef UNSUPPORTED_SOCKET_OPTION_NOT_PREDEFINED
#undef UNSUPPORTED_SOCKET_OPTION_NOT_PREDEFINED
#undef UNSUPPORTED_SOCKET_OPTION
#endif

#ifndef SUPPORTED_SOCKET_PROTOCOL
#define SUPPORTED_SOCKET_PROTOCOL(name)
#define SUPPORTED_SOCKET_PROTOCOL_NOT_PREDEFINED
#endif

#ifndef UNSUPPORTED_SOCKET_PROTOCOL
#define UNSUPPORTED_SOCKET_PROTOCOL(name)
#define UNSUPPORTED_SOCKET_PROTOCOL_NOT_PREDEFINED
#endif

SUPPORTED_SOCKET_PROTOCOL(IPPROTO_IP)
// Same value as IPPROTO_IP
//SUPPORTED_SOCKET_PROTOCOL(IPPROTO_HOPOPTS)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_ICMP)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_IGMP)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_TCP)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_UDP)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_IPV6)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_ROUTING)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_FRAGMENT)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_ESP)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_AH)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_ICMPV6)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_NONE)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_DSTOPTS)
SUPPORTED_SOCKET_PROTOCOL(IPPROTO_RAW)

UNSUPPORTED_SOCKET_PROTOCOL(IPPROTO_ETHERIP)

#ifdef SUPPORTED_SOCKET_PROTOCOL_NOT_PREDEFINED
#undef SUPPORTED_SOCKET_PROTOCOL_NOT_PREDEFINED
#undef SUPPORTED_SOCKET_PROTOCOL
#endif

#ifdef UNSUPPORTED_SOCKET_PROTOCOL_NOT_PREDEFINED
#undef UNSUPPORTED_SOCKET_PROTOCOL_NOT_PREDEFINED
#undef UNSUPPORTED_SOCKET_PROTOCOL
#endif

#ifndef SUPPORTED_SEND_MESSAGE_FLAG
#define SUPPORTED_SEND_MESSAGE_FLAG(name)
#define SUPPORTED_SEND_MESSAGE_FLAG_NOT_PREDEFINED
#endif

#ifndef UNSUPPORTED_SEND_MESSAGE_FLAG
#define UNSUPPORTED_SEND_MESSAGE_FLAG(name)
#define UNSUPPORTED_SEND_MESSAGE_FLAG_NOT_PREDEFINED
#endif

SUPPORTED_SEND_MESSAGE_FLAG(MSG_OOB)
SUPPORTED_SEND_MESSAGE_FLAG(MSG_PEEK)
SUPPORTED_SEND_MESSAGE_FLAG(MSG_DONTROUTE)
SUPPORTED_SEND_MESSAGE_FLAG(MSG_EOR)
SUPPORTED_SEND_MESSAGE_FLAG(MSG_TRUNC)
SUPPORTED_SEND_MESSAGE_FLAG(MSG_CTRUNC)
SUPPORTED_SEND_MESSAGE_FLAG(MSG_WAITALL)
SUPPORTED_SEND_MESSAGE_FLAG(MSG_DONTWAIT)
SUPPORTED_SEND_MESSAGE_FLAG(MSG_NOSIGNAL)

UNSUPPORTED_SEND_MESSAGE_FLAG(MSG_BCAST)
UNSUPPORTED_SEND_MESSAGE_FLAG(MSG_MCAST)
// Emulated through MSG_FIN
//UNSUPPORTED_SEND_MESSAGE_FLAG(MSG_EOF)

#ifdef SUPPORTED_SEND_MESSAGE_FLAG_NOT_PREDEFINED
#undef SUPPORTED_SEND_MESSAGE_FLAG_NOT_PREDEFINED
#undef SUPPORTED_SEND_MESSAGE_FLAG
#endif

#ifdef UNSUPPORTED_SEND_MESSAGE_FLAG_NOT_PREDEFINED
#undef UNSUPPORTED_SEND_MESSAGE_FLAG_NOT_PREDEFINED
#undef UNSUPPORTED_SEND_MESSAGE_FLAG
#endif

#ifndef SUPPORTED_TCP_OPTION
#define SUPPORTED_TCP_OPTION(name)
#define SUPPORTED_TCP_OPTION_NOT_PREDEFINED
#endif

#ifndef UNSUPPORTED_TCP_OPTION
#define UNSUPPORTED_TCP_OPTION(name)
#define UNSUPPORTED_TCP_OPTION_NOT_PREDEFINED
#endif

SUPPORTED_TCP_OPTION(TCP_NODELAY)
SUPPORTED_TCP_OPTION(TCP_MAXSEG)

UNSUPPORTED_TCP_OPTION(TCP_NOPUSH)
UNSUPPORTED_TCP_OPTION(TCP_NOOPT)

#ifdef SUPPORTED_TCP_OPTION_NOT_PREDEFINED
#undef SUPPORTED_TCP_OPTION_NOT_PREDEFINED
#undef SUPPORTED_TCP_OPTION
#endif

#ifdef UNSUPPORTED_TCP_OPTION_NOT_PREDEFINED
#undef UNSUPPORTED_TCP_OPTION_NOT_PREDEFINED
#undef UNSUPPORTED_TCP_OPTION
#endif