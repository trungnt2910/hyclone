#include <cstring>
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

size_t loader_system_timezone(int* offset, char* buffer, size_t buffer_size)
{
    time_t current_time = time(NULL);
    struct tm time_info;
    localtime_r(&current_time, &time_info);

    size_t length = strlen(time_info.tm_zone);
    if (buffer != NULL)
    {
        strncpy(buffer, time_info.tm_zone, buffer_size);
    }

    if (offset != NULL)
    {
        *offset = time_info.tm_gmtoff;
    }

    return length;
}