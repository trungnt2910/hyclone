// Must be included before signal.h
#include "haiku_signal.h"

#include <cstddef>
#include <linux/resource.h>
#include <memory.h>
#include <signal.h>
#include <wait.h>

#include "BeDefs.h"
#include "errno_conversion.h"
#include "export.h"
#include "extended_commpage.h"
#include "linux_syscall.h"
#include "signal_conversion.h"
#include "syscall_load_image.h"

enum
{
    /* compatible to sys/resource.h RUSAGE_SELF and RUSAGE_CHILDREN */
    B_TEAM_USAGE_SELF       = 0,
    B_TEAM_USAGE_CHILDREN   = -1
};

/* waitpid()/waitid() options */
#define HAIKU_WNOHANG       0x01
#define HAIKU_WUNTRACED     0x02
#define HAIKU_WCONTINUED    0x04
#define HAIKU_WEXITED       0x08
#define HAIKU_WSTOPPED      0x10
#define HAIKU_WNOWAIT       0x20

enum which_process_info
{
    SESSION_ID = 1,
    GROUP_ID,
    PARENT_ID,
};

extern "C"
{

void MONIKA_EXPORT _kern_exit_team(status_t returnValue)
{
    if (__gCommPageAddress != NULL)
    {
        auto at_exit_hook = GET_HOSTCALLS()->at_exit;
        if (at_exit_hook != NULL)
            at_exit_hook(returnValue);
    }

    LINUX_SYSCALL1(__NR_exit, returnValue);
}

thread_id MONIKA_EXPORT _kern_fork()
{
    CHECK_COMMPAGE();
    thread_id id = GET_HOSTCALLS()->fork();

    if (id < 0)
    {
        return LinuxToB(-id);
    }

    if (id > 0)
    {
        long status = GET_SERVERCALLS()->fork(id);
        if (status < 0)
        {
            LINUX_SYSCALL2(__NR_kill, id, SIGKILL);
            return status;
        }
    }
    else
    {
        GET_SERVERCALLS()->wait_for_fork_unlock();
    }

    return id;
}

int MONIKA_EXPORT _kern_exec(const char *path, const char* const* flatArgs,
    size_t flatArgsSize, int32 argCount, int32 envCount, haiku_mode_t umask)
{
    CHECK_COMMPAGE();

    long status = GET_HOSTCALLS()->exec(path, flatArgs, flatArgsSize, argCount, envCount, umask);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    // Should never reach here.
    return status;
}

thread_id MONIKA_EXPORT _kern_load_image(const char* const* flatArgs,
    size_t flatArgsSize, int32 argCount, int32 envCount,
    int32 priority, uint32 flags, port_id errorPort,
    uint32 errorToken)
{
    if (argCount < 1)
    {
        return B_BAD_VALUE;
    }

    long status = GET_HOSTCALLS()->spawn(flatArgs[0], flatArgs, flatArgsSize, argCount, envCount, priority, flags, errorPort, errorToken);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    int pid = status;

    if (flags & B_WAIT_TILL_LOADED)
    {
        status = GET_SERVERCALLS()->wait_for_app_load(pid);

        if (status < 0)
        {
            return status;
        }
    }

    return pid;
}

haiku_pid_t MONIKA_EXPORT _kern_wait_for_child(thread_id child, uint32 flags,
    haiku_siginfo_t* info, team_usage_info* usageInfo)
{
    int linuxFlags = 0;
    if (flags & HAIKU_WNOHANG)
    {
        linuxFlags |= WNOHANG;
    }
    if (flags & HAIKU_WUNTRACED)
    {
        linuxFlags |= WUNTRACED;
    }
    if (flags & HAIKU_WCONTINUED)
    {
        linuxFlags |= WCONTINUED;
    }
    if (flags & HAIKU_WEXITED)
    {
        linuxFlags |= WEXITED;
    }
    if (flags & HAIKU_WSTOPPED)
    {
        linuxFlags |= WSTOPPED;
    }
    if (flags & HAIKU_WNOWAIT)
    {
        linuxFlags |= WNOWAIT;
    }

    struct rusage linuxUsageInfo;
    siginfo_t linuxInfo;

    // Initialize this structure else it'll return crap.
    memset(&linuxInfo, 0, sizeof(linuxInfo));

    // Always pass in linuxInfo for the pid information.
    long status = LINUX_SYSCALL5(__NR_waitid, child > 0 ? P_PID : P_ALL, child,
        &linuxInfo, linuxFlags, (usageInfo == NULL) ? NULL : &linuxUsageInfo);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    if (info != NULL)
    {
        SiginfoLinuxToB(linuxInfo, *info);
    }

    if (usageInfo != NULL)
    {
        // According to https://github.com/pdziepak/Haiku/blob/9c5c5990414f9570d5271328ce67252d50dfe8a2/src/libs/bsd/wait.c#L33,
        // it looks like Haiku's struct uses microseconds for the fields.
        usageInfo->user_time = linuxUsageInfo.ru_utime.tv_sec * 1000000 + linuxUsageInfo.ru_utime.tv_usec;
        usageInfo->kernel_time = linuxUsageInfo.ru_stime.tv_sec * 1000000 + linuxUsageInfo.ru_stime.tv_usec;
    }

    // Status does not contain the child PID, but
    // is always 0 on Linux.
    return linuxInfo.si_pid;
}

status_t MONIKA_EXPORT _kern_get_team_usage_info(team_id team, int32 who, team_usage_info *info, size_t size)
{
    if (size != sizeof(team_usage_info))
    {
        return B_BAD_VALUE;
    }

    switch (who)
    {
        case B_TEAM_USAGE_SELF:
        case B_TEAM_USAGE_CHILDREN:
            break;
        default:
            return B_BAD_VALUE;
    }

    CHECK_COMMPAGE();

    return GET_HOSTCALLS()->get_process_usage(team, info);
}

haiku_pid_t _kern_process_info(haiku_pid_t process, int32 which)
{
    long status;
    switch (which)
    {
        case SESSION_ID:
            status = LINUX_SYSCALL1(__NR_getsid, process);
            break;
        case GROUP_ID:
            status = LINUX_SYSCALL1(__NR_getpgid, process);
            break;
        case PARENT_ID:
            status = LINUX_SYSCALL1(__NR_getppid, process);
            break;
        default:
            return B_BAD_VALUE;
            break;
    }

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return status;
}


haiku_pid_t _kern_setpgid(haiku_pid_t process, haiku_pid_t group)
{
    long result = LINUX_SYSCALL2(__NR_setpgid, process, group);
    if (result < 0)
    {
        return LinuxToB(-result);
    }

    result = LINUX_SYSCALL1(__NR_getpgid, process);
    if (result < 0)
    {
        return LinuxToB(-result);
    }

    return result;
}

}