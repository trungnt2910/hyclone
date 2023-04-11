#include <cstddef>
#include <time.h>

#include "BeDefs.h"
#include "errno_conversion.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "haiku_time.h"
#include "linux_syscall.h"

extern "C"
{

// Fills the time zone offset in seconds in the field _timezoneOffset,
// and the name of the timezone in the buffer by name.
status_t _moni_get_timezone(int32 *_timezoneOffset, char *name, size_t nameLength)
{
    if (GET_HOSTCALLS()->system_timezone(_timezoneOffset, name, nameLength) < 0)
    {
        return B_ERROR;
    }

    return B_OK;
}

status_t _moni_get_clock(clockid_t clockID, bigtime_t* _time)
{
    long status;

    if (_time == NULL)
    {
        return B_BAD_ADDRESS;
    }

    if (clockID > 0)
    {
        // The clock ID is a ID of the team whose CPU time the clock refers to.
        return HAIKU_POSIX_ENOSYS;
    }
    else
    {
        int linuxClockID = -1;

        switch (clockID)
        {
            case HAIKU_CLOCK_MONOTONIC:
            {
                linuxClockID = CLOCK_MONOTONIC;
                break;
            }
            case HAIKU_CLOCK_REALTIME:
            {
                linuxClockID = CLOCK_REALTIME;
                break;
            }
            case HAIKU_CLOCK_PROCESS_CPUTIME_ID:
            {
                linuxClockID = CLOCK_PROCESS_CPUTIME_ID;
                break;
            }
            case HAIKU_CLOCK_THREAD_CPUTIME_ID:
            {
                linuxClockID = CLOCK_THREAD_CPUTIME_ID;
                break;
            }
            default:
            {
                return B_BAD_VALUE;
            }
        }

        struct timespec linuxTime;
        status = LINUX_SYSCALL2(__NR_clock_gettime, linuxClockID, &linuxTime);
        if (status < 0)
        {
            return LinuxToB(-status);
        }

        *_time = linuxTime.tv_sec * 1000000LL + linuxTime.tv_nsec / 1000LL;

        return B_OK;
    }
}

}