#include <string.h>
#include <sys/utsname.h>

#include "linux_syscall.h"
#include "linux_version.h"

bool is_linux_version_at_least(int major, int minor)
{
    struct utsname uname_info;

    long status = LINUX_SYSCALL1(__NR_uname, &uname_info);

    if (status < 0)
    {
        return false;
    }

    int kernelMajor = 0;
    int kernelMinor = 0;

    size_t pos = 0;

    while (uname_info.release[pos] != '\0' && uname_info.release[pos] != '.')
    {
        kernelMajor = kernelMajor * 10 + uname_info.release[pos] - '0';
        ++pos;
    }

    ++pos;

    while (uname_info.release[pos] != '\0' && uname_info.release[pos] != '.')
    {
        kernelMinor = kernelMinor * 10 + uname_info.release[pos] - '0';
        ++pos;
    }

    if (kernelMajor != major)
    {
        return kernelMajor > major;
    }

    return kernelMajor >= minor;
}