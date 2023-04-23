#include <errno.h>
#include <unistd.h>

#include "BeDefs.h"
#include "errno_conversion.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "linux_syscall.h"

extern "C"
{

haiku_gid_t _moni_getgid(bool effective)
{
    return GET_SERVERCALLS()->getgid(effective);
}

haiku_uid_t _moni_getuid(bool effective)
{
    return GET_SERVERCALLS()->getuid(effective);
}

status_t _moni_setregid(haiku_gid_t rgid, haiku_gid_t egid,
    bool setAllIfPrivileged)
{
    intptr_t hostrgid = -1;
    intptr_t hostegid = -1;
    status_t status = GET_SERVERCALLS()->setregid(rgid, egid, setAllIfPrivileged, &hostrgid, &hostegid);

    if (status < 0)
    {
        return status;
    }

    if (hostrgid != -1 || hostegid != -1)
    {
        long result = LINUX_SYSCALL2(__NR_setregid, hostrgid, hostegid);
        if (result < 0)
        {
            return LinuxToB(-result);
        }

        // Retry
        status = GET_SERVERCALLS()->setregid(rgid, egid, setAllIfPrivileged, NULL, NULL);
    }

    if (status < 0)
    {
        return status;
    }

    return B_OK;
}

status_t _moni_setreuid(haiku_uid_t ruid, haiku_uid_t euid,
    bool setAllIfPrivileged)
{
    intptr_t hostruid = -1;
    intptr_t hosteuid = -1;
    status_t status = GET_SERVERCALLS()->setreuid(ruid, euid, setAllIfPrivileged, &hostruid, &hosteuid);

    if (status < 0)
    {
        return status;
    }

    if (hostruid != -1 || hosteuid != -1)
    {
        long result = LINUX_SYSCALL2(__NR_setreuid, hostruid, hosteuid);
        if (result < 0)
        {
            return LinuxToB(-result);
        }

        // Retry
        status = GET_SERVERCALLS()->setreuid(ruid, euid, setAllIfPrivileged, NULL, NULL);
    }

    if (status < 0)
    {
        return status;
    }

    return B_OK;
}

haiku_ssize_t _moni_getgroups(int groupCount, haiku_gid_t* groupList)
{
    static_assert(sizeof(haiku_gid_t) == sizeof(int), "gid_t is not 32-bit.");
    return GET_SERVERCALLS()->getgroups(groupCount, (int*)groupList);
}

status_t _moni_setgroups(int groupCount, haiku_gid_t* groupList)
{
    static_assert(sizeof(haiku_gid_t) == sizeof(int), "gid_t is not 32-bit.");

    if (groupCount > 0)
    {
        intptr_t* hostGroupList = (intptr_t*)__builtin_alloca(groupCount * sizeof(intptr_t));
        long hostGroupCount = GET_SERVERCALLS()->setgroups(groupCount, (int*)groupList, hostGroupList);

        if (hostGroupCount < 0)
        {
            return hostGroupCount;
        }

        if (hostGroupCount > 0)
        {
            gid_t* hostGroupList32 = (gid_t*)__builtin_alloca(hostGroupCount * sizeof(gid_t));
            for (long i = 0; i < hostGroupCount; i++)
            {
                hostGroupList32[i] = hostGroupList[i];
            }

            long status = LINUX_SYSCALL2(__NR_setgroups, hostGroupCount, hostGroupList32);
            if (status < 0)
            {
                return LinuxToB(-status);
            }
        }
    }

    return GET_SERVERCALLS()->setgroups(groupCount, (int*)groupList, NULL);
}

}