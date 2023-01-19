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

#define HAIKU_SEEK_SET 0
#define HAIKU_SEEK_CUR 1
#define HAIKU_SEEK_END 2

int SeekTypeBToLinux(int seekType)
{
    switch (seekType)
    {
    case HAIKU_SEEK_SET:
        return SEEK_SET;
    case HAIKU_SEEK_CUR:
        return SEEK_CUR;
    case HAIKU_SEEK_END:
        return SEEK_END;
    default:
        panic("SeekTypeBToLinux: Unknown seek type.");
    }
}

int SeekTypeLinuxToB(int seekType)
{
    switch (seekType)
    {
    case SEEK_SET:
        return HAIKU_SEEK_SET;
    case SEEK_CUR:
        return HAIKU_SEEK_CUR;
    case SEEK_END:
        return HAIKU_SEEK_END;
    default:
        panic("SeekTypeLinuxToB: Unknown seek type.");
    }
}