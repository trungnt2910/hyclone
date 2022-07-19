#ifndef __LINUX_ERRNO_CONVERSION__
#define __LINUX_ERRNO_CONVERSION__

int BToLinux(int bErrno);
int LinuxToB(int linuxErrno);

#endif // __LINUX_ERRNO_CONVERSION__