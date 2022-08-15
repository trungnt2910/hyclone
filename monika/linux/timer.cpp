#include <signal.h>
#include <time.h>

#include "BeDefs.h"
#include "export.h"
#include "extended_commpage.h"
#include "linux_cpu_timers.h"
#include "linux_subsystemlock.h"
#include "linux_syscall.h"
#include "user_time_defs.h"

static int32_t sTimerSubsystemLock = HYCLONE_SUBSYSTEM_LOCK_UNINITIALIZED;
static void* sTimerIdMap = NULL;

void InitializeTimerIdMap();

extern "C"
{

status_t MONIKA_EXPORT _kern_set_timer(int32 timerID, thread_id threadID,
    bigtime_t startTime, bigtime_t interval,
    uint32 flags, struct user_timer_info* oldInfo)
{
    // Setting for a specific thread id is not supported.
    (void)threadID;

    timer_t linuxTimer;
    {
        auto lock = SubsystemLock(sTimerSubsystemLock);
        if (sTimerIdMap == NULL)
        {
            InitializeTimerIdMap();
        }
        linuxTimer = (timer_t)GET_HOSTCALLS()->idmap_get(sTimerIdMap, timerID);
    }

    if (linuxTimer == (timer_t)0)
    {
        return HAIKU_POSIX_EINVAL;
    }

    if (timerID < USER_TIMER_FIRST_USER_DEFINED_ID && linuxTimer == (timer_t)-1)
    {
        return HAIKU_POSIX_ENOSYS;
    }

    struct itimerspec linuxNewInfo;
    linuxNewInfo.it_value.tv_sec = startTime / 1000000;
    linuxNewInfo.it_value.tv_nsec = (startTime % 1000000) * 1000;
    linuxNewInfo.it_interval.tv_sec = interval / 1000000;
    linuxNewInfo.it_interval.tv_nsec = (interval % 1000000) * 1000;

    int linuxFlags = 0;
    if (flags & B_ABSOLUTE_TIMEOUT)
        linuxFlags |= TIMER_ABSTIME;

    struct itimerspec linuxOldInfo;
    LINUX_SYSCALL4(__NR_timer_settime, linuxTimer, linuxFlags, &linuxNewInfo, &linuxOldInfo);

    if (oldInfo != NULL)
    {
        oldInfo->remaining_time = linuxOldInfo.it_value.tv_sec * 1000000 + linuxOldInfo.it_value.tv_nsec / 1000;
        oldInfo->interval = linuxOldInfo.it_interval.tv_sec * 1000000 + linuxOldInfo.it_interval.tv_nsec / 1000;
        // This value is a stub.
        oldInfo->overrun_count = 0;
    }

    return B_OK;
}

}

void InitializeTimerIdMap()
{
    sTimerIdMap = GET_HOSTCALLS()->idmap_create();

    // Add a few system defined timers
    timer_t realTimeTimer, teamTotalTimeTimer, teamUserTimeTimer;
    struct sigevent linuxSigevent;
    linuxSigevent.sigev_notify = SIGEV_SIGNAL;
    linuxSigevent.sigev_signo = SIGALRM;

    long status;

    linuxSigevent.sigev_value.sival_int = USER_TIMER_REAL_TIME_ID;
    LINUX_SYSCALL3(__NR_timer_create, CLOCK_REALTIME, &linuxSigevent, &realTimeTimer);

    linuxSigevent.sigev_value.sival_int = USER_TIMER_TEAM_TOTAL_TIME_ID;
    status = LINUX_SYSCALL3(__NR_timer_create, MAKE_PROCESS_CPUCLOCK(0, CPUCLOCK_SCHED), &linuxSigevent, &teamTotalTimeTimer);

    // This happens on WSL1, where CLOCK_PROCESS_CPUTIME_ID is not supported.
    if (status < 0)
    {
        teamTotalTimeTimer = (timer_t)-1;
    }

    linuxSigevent.sigev_value.sival_int = USER_TIMER_TEAM_USER_TIME_ID;
    status = LINUX_SYSCALL3(__NR_timer_create, MAKE_PROCESS_CPUCLOCK(0, CPUCLOCK_VIRT), &linuxSigevent, &teamUserTimeTimer);

    if (status < 0)
    {
        teamUserTimeTimer = (timer_t)-1;
    }

    // First three entries.
    GET_HOSTCALLS()->idmap_add(sTimerIdMap, realTimeTimer);
    GET_HOSTCALLS()->idmap_add(sTimerIdMap, teamTotalTimeTimer);
    GET_HOSTCALLS()->idmap_add(sTimerIdMap, teamUserTimeTimer);
}
