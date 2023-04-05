#ifndef __SERVER_ERRNO_H__
#define __SERVER_ERRNO_H__

#include <system_error>

int LinuxToB(int linuxErrno);
int CppToB(const std::error_code& ec);

#endif // __SERVER_ERRNO_H__