#include <errno.h>

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

haiku_ssize_t _moni_getgroups(int groupCount, haiku_gid_t* groupList)
{
    long result = LINUX_SYSCALL2(__NR_getgroups, groupCount, groupList);
    if (result < 0)
    {
        return LinuxToB(-result);
    }

    if (groupList != NULL)
    {
        for (int i = 0; i < result; i++)
        {
            groupList[i] = GET_SERVERCALLS()->gid_for(groupList[i]);
        }
    }

    return result;
}

status_t _moni_setgroups(int groupCount, haiku_gid_t* groupList)
{
    if (groupList != NULL)
    {
        for (int i = 0; i < groupCount; i++)
        {
            groupList[i] = GET_SERVERCALLS()->hostgid_for(groupList[i]);
        }
    }

    long result = LINUX_SYSCALL2(__NR_setgroups, groupCount, groupList);
    if (result < 0)
    {
        if (result == -EPERM && GET_SERVERCALLS()->getuid(true) == 0)
        {
            // TODO: Store these groups on hyclone_server side.
            GET_SERVERCALLS()->debug_output("setgroup() failed with EPERM but we're emulating root.",
                sizeof("setgroup() failed with EPERM but we're emulating root."));
            return B_OK;
        }
        return LinuxToB(-result);
    }
}

}