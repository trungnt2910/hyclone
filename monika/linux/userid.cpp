#include "BeDefs.h"
#include "errno_conversion.h"
#include "extended_commpage.h"
#include "export.h"
#include "haiku_errors.h"
#include "linux_syscall.h"

extern "C"
{

haiku_gid_t MONIKA_EXPORT _kern_getgid(bool effective)
{
    return GET_SERVERCALLS()->getgid(effective);
}

haiku_uid_t MONIKA_EXPORT _kern_getuid(bool effective)
{
    return GET_SERVERCALLS()->getuid(effective);
}

haiku_ssize_t MONIKA_EXPORT _kern_getgroups(int groupCount, haiku_gid_t* groupList)
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

}