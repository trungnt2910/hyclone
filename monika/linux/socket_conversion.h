#ifndef __SOCKET_CONVERSION_H__
#define __SOCKET_CONVERSION_H__

#include <sys/socket.h>
#include "haiku_socket.h"

int SocketFamilyBToLinux(int family);
int SocketTypeBToLinux(int type);
int SocketTypeLinuxToB(int type);
int SocketProtocolBToLinux(int protocol);
int SocketOptionBToLinux(int option);
int SocketAddressBToLinux(const struct haiku_sockaddr *addr, haiku_socklen_t addrlen,
                                  struct sockaddr_storage *storage);
int SocketAddressLinuxToB(const struct sockaddr *addr, struct haiku_sockaddr_storage *storage);

#endif // __SOCKET_CONVERSION_H__