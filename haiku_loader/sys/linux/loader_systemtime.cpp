#include <time.h>
#include "loader_systemtime.h"

int64_t loader_system_time()
{
    struct timespec tp;
    clock_gettime(CLOCK_BOOTTIME, &tp);

    return tp.tv_sec * 1000000 + tp.tv_nsec / 1000;
}

int64_t loader_system_time_nsecs()
{
    struct timespec tp;
    clock_gettime(CLOCK_BOOTTIME, &tp);

    return tp.tv_sec * 1000000000 + tp.tv_nsec;
}