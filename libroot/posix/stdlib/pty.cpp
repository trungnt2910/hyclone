#include "extended_commpage.h"

extern "C"
{

int
posix_openpt(int openFlags)
{
    return GET_HOSTCALLS()->posix_openpt(openFlags);
}

int
grantpt(int masterFD)
{
    return GET_HOSTCALLS()->grantpt(masterFD);
}

char*
ptsname(int masterFD)
{
    static char buffer[32];

    if (GET_HOSTCALLS()->ptsname(masterFD, buffer, sizeof(buffer)) != 0)
    {
        return NULL;
    }

    return buffer;
}

int
unlockpt(int masterFD)
{
    return GET_HOSTCALLS()->unlockpt(masterFD);
}

}