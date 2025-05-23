#include <cstring>
#include <time.h>

#include "BeDefs.h"
#include "errno_conversion.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "haiku_semaphore.h"
#include "haiku_thread.h"
#include "haiku_time.h"
#include "haiku_tls.h"
#include "linux_debug.h"
#include "linux_syscall.h"
#include "user_thread_defs.h"

extern "C"
{

thread_id _moni_spawn_thread(void* attributes)
{
    long status = GET_HOSTCALLS()->spawn_thread(attributes);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return status;
}

status_t _moni_suspend_thread(thread_id thread)
{
    return GET_SERVERCALLS()->suspend_thread(thread);
}
status_t _moni_resume_thread(thread_id thread)
{
    return GET_SERVERCALLS()->resume_thread(thread);
}

status_t _moni_block_thread(uint32 flags, bigtime_t timeout)
{
    return GET_SERVERCALLS()->block_thread(flags, timeout);
}

status_t _moni_unblock_thread(thread_id thread, status_t status)
{
    return GET_SERVERCALLS()->unblock_thread(thread, status);
}

status_t _moni_send_data(thread_id thread, int32 code, const void* buffer, size_t size)
{
    return GET_SERVERCALLS()->send_data(thread, code, buffer, size);
}

int32 _moni_receive_data(thread_id* _sender, void* buffer, size_t size)
{
    return GET_SERVERCALLS()->receive_data(_sender, buffer, size);
}

status_t _moni_exit_thread(status_t returnValue)
{
    GET_HOSTCALLS()->exit_thread(returnValue);
    return 0;
}

void _moni_thread_yield(void)
{
    LINUX_SYSCALL0(__NR_sched_yield);
}

status_t _moni_wait_for_thread_etc(thread_id thread, uint32 flags, bigtime_t timeout,
    status_t* _returnCode)
{
    long status = GET_HOSTCALLS()->wait_for_thread(thread, flags, timeout, _returnCode);
    if (status < 0)
    {
        return LinuxToB(-status);
    }
    return 0;
}

void _moni_find_thread()
{
    panic("_kern_find_thread not implemented");
}

status_t _moni_get_thread_info(thread_id id, haiku_thread_info* info)
{
    return GET_SERVERCALLS()->get_thread_info(id, info);
}

status_t _moni_get_next_thread_info(team_id team, int32* cookie,
    haiku_thread_info* info)
{
    return GET_SERVERCALLS()->get_next_thread_info(team, cookie, info);
}

status_t _moni_rename_thread(thread_id thread, const char* newName)
{
    return GET_SERVERCALLS()->rename_thread(thread, newName, strlen(newName));
}

status_t _moni_set_thread_priority(thread_id id, int32 newPriority)
{
    // This is a privileged syscall so we'll have to rely on the server.
    return GET_SERVERCALLS()->set_thread_priority(id, newPriority);
}

status_t _moni_mutex_lock(int32* mutex, const char* name,
    uint32 flags, bigtime_t timeout)
{
    return GET_HOSTCALLS()->mutex_lock(mutex, name, flags, timeout);
}

status_t _moni_mutex_unblock(int32* mutex, uint32 flags)
{
    return GET_HOSTCALLS()->mutex_unblock(mutex, flags);
}

status_t _moni_mutex_switch_lock(int32* fromMutex, int32* toMutex,
    const char* name, uint32 flags, bigtime_t timeout)
{
    return GET_HOSTCALLS()->mutex_switch_lock(fromMutex, toMutex, name, flags, timeout);
}

sem_id _moni_create_sem(int count, const char *name)
{
    return GET_SERVERCALLS()->create_sem(count, name, (name == NULL) ? 0 : strlen(name));
}

status_t _moni_acquire_sem(sem_id id)
{
    return GET_SERVERCALLS()->acquire_sem(id);
}

status_t _moni_acquire_sem_etc(sem_id id, uint32 count, uint32 flags,
    bigtime_t timeout)
{
    return GET_SERVERCALLS()->acquire_sem_etc(id, count, flags, timeout);
}

status_t _moni_release_sem(sem_id id)
{
    return GET_SERVERCALLS()->release_sem(id);
}

status_t _moni_release_sem_etc(sem_id id, uint32 count, uint32 flags)
{
    return GET_SERVERCALLS()->release_sem_etc(id, count, flags);
}

status_t _moni_delete_sem(sem_id id)
{
    return GET_SERVERCALLS()->delete_sem(id);
}

status_t _moni_get_sem_count(sem_id id, int32* thread_count)
{
    return GET_SERVERCALLS()->get_sem_count(id, thread_count);
}

status_t _moni_realtime_sem_open(const char* name, int openFlagsOrShared,
    haiku_mode_t mode, uint32 semCount, haiku_sem_t* userSem, haiku_sem_t** _usedUserSem)
{
    long status = GET_HOSTCALLS()->realtime_sem_open(name, openFlagsOrShared, mode, semCount, userSem, _usedUserSem);
    if (status < 0)
    {
        return LinuxToB(-status);
    }
    return 0;
}

status_t _moni_snooze_etc(bigtime_t time, int timebase, int32 flags, bigtime_t* _remainingTime)
{
    int linuxClockid;
    switch (timebase)
    {
        static_assert(B_SYSTEM_TIMEBASE == HAIKU_CLOCK_MONOTONIC, "Something in Haiku's ABI has changed.");
        // case HAIKU_CLOCK_MONOTONIC
        case B_SYSTEM_TIMEBASE:
            linuxClockid = CLOCK_MONOTONIC;
        break;
        case HAIKU_CLOCK_REALTIME:
            linuxClockid = CLOCK_REALTIME;
        break;
        case HAIKU_CLOCK_PROCESS_CPUTIME_ID:
            linuxClockid = CLOCK_PROCESS_CPUTIME_ID;
        break;
        case HAIKU_CLOCK_THREAD_CPUTIME_ID:
            linuxClockid = CLOCK_THREAD_CPUTIME_ID;
        default:
            trace("_kern_snooze_etc: unknown timebase.");
            return B_BAD_VALUE;
        break;
    }

    int linuxFlags = 0;

    if (flags == B_ABSOLUTE_TIMEOUT)
    {
        linuxFlags = TIMER_ABSTIME;
    }

    struct timespec linuxRequest;
    struct timespec linuxRemaining;

    linuxRequest.tv_sec = time / 1000000;
    linuxRequest.tv_nsec = (time % 1000000) * 1000;

    long status = LINUX_SYSCALL4(__NR_clock_nanosleep, linuxClockid, linuxFlags, &linuxRequest, &linuxRemaining);

    if (_remainingTime != NULL)
    {
        *_remainingTime = linuxRemaining.tv_sec * 1000000 + linuxRemaining.tv_nsec / 1000;
    }

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

}
