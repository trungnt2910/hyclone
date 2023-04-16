#include <cstdlib>
#include <fcntl.h>

#include "haiku_fcntl.h"
#include "loader_pty.h"

int loader_openpt(int openFlags)
{
    int linuxOpenFlags = 0;

    if (openFlags & HAIKU_O_RDWR)
    {
        linuxOpenFlags |= O_RDWR;
    }

    if (openFlags & HAIKU_O_NOCTTY)
    {
        linuxOpenFlags |= O_NOCTTY;
    }

    return posix_openpt(linuxOpenFlags);
}

int loader_grantpt(int masterFD)
{
    return grantpt(masterFD);
}

int loader_ptsname(int masterFD, char* buffer, size_t bufferSize)
{
    return ptsname_r(masterFD, buffer, bufferSize);
}

int loader_unlockpt(int masterFD)
{
    return unlockpt(masterFD);
}