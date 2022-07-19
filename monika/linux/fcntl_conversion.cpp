#include <fcntl.h>
#include "fcntl_conversion.h"
#include "haiku_fcntl.h"
#include "linux_debug.h"

#include "extended_commpage.h"

#ifndef O_NOCACHE
#define O_NOCACHE O_DIRECT
#endif

int OFlagsBToLinux(int flags)
{
    int linuxFlags = 0;

    switch (flags & HAIKU_O_ACCMODE)
    {
#define SUPPORTED_RWOFLAG(flag)                                 \
        case HAIKU_##flag: linuxFlags |= flag; break;
#include "fcntl_values.h"
#undef SUPPORTED_RWOFLAG
    }

#define SUPPORTED_OFLAG(name)                                   \
    if (flags & HAIKU_##name)                                   \
        linuxFlags |= name;
#define UNSUPPORTED_OFLAG(name)                                 \
    if (flags & HAIKU_##name)                                   \
        trace("OFlagsBToLinux: unsupported flag: "#name);

#include "fcntl_values.h"

#undef SUPPORTED_OFLAG
#undef UNSUPPORTED_OFLAG

    return linuxFlags;
}

int OFlagsLinuxToB(int flags)
{
    int haikuFlags = 0;

    switch (flags & O_ACCMODE)
    {
#define SUPPORTED_RWOFLAG(flag)                                 \
        case flag: haikuFlags |= HAIKU_##flag; break;
#include "fcntl_values.h"
#undef SUPPORTED_RWOFLAG
    }

#define SUPPORTED_OFLAG(name)                                   \
    if (flags & name)                                           \
        haikuFlags |= HAIKU_##name;

#include "fcntl_values.h"

#undef SUPPORTED_OFLAG

    return haikuFlags;
}