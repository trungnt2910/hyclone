#include "BeDefs.h"
#include "errno_conversion.h"
#include "export.h"
#include "haiku_errors.h"
#include "linux_syscall.h"

extern "C"
{

// The current implementation trivially reflects the uid and gid
// of the current user in Linux.
//
// However, as the project continues, this may prove to be
// ineffective (for example, Linux root access is not required
// for users to edit files from the Haiku prefix). Furthermore,
// many Haiku applications are used to be run as uid 0 (root).
//
// We may replace these syscalls with calls to hyclone_server
// and manage these IDs in a way similar to what Darling does.
haiku_gid_t MONIKA_EXPORT _kern_getgid(bool effective)
{
    return 0;
    // long result = LINUX_SYSCALL0(effective ? __NR_getegid : __NR_getgid);
    // if (result < 0)
    // {
    //     return LinuxToB(-result);
    // }

    // return result;
}

haiku_uid_t MONIKA_EXPORT _kern_getuid(bool effective)
{
    return 0;
    // long result = LINUX_SYSCALL0(effective ? __NR_geteuid : __NR_getuid);
    // if (result < 0)
    // {
    //     return LinuxToB(-result);
    // }

    // return result;
}

haiku_ssize_t MONIKA_EXPORT _kern_getgroups(int groupCount, haiku_gid_t* groupList)
{
    long result = LINUX_SYSCALL2(__NR_getgroups, groupCount, groupList);
    if (result < 0)
    {
        return LinuxToB(-result);
    }

    return result;
}

}