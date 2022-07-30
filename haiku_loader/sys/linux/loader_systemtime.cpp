#include <climits>
#include <cstring>
#include <fstream>
#include <sys/sysinfo.h>
#include <time.h>
#include <unistd.h>
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

int64_t loader_system_time_offset()
{
    // Reading the btime field from /proc/stat
    // is also possible but slower.
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);

    // current time - up time.
    return (tp.tv_sec * 1000000 + tp.tv_nsec / 1000) - loader_system_time();
}

size_t loader_system_timezone(int* offset, char* buffer, size_t buffer_size)
{
    time_t current_time = time(NULL);
    struct tm time_info;
    localtime_r(&current_time, &time_info);

    std::string timezoneName;


    // Method 1: /etc/timezone (available on some Linux distros)
    {
        std::ifstream fin("/etc/timezone");
        if (fin.is_open())
        {
            fin >> timezoneName;
        }
    }

    // Method 2: /etc/localtime
    if (timezoneName.empty())
    {
        char buffer[PATH_MAX];
        ssize_t count = readlink("/etc/localtime", buffer, PATH_MAX);

        if (count > 0)
        {
            buffer[count] = '\0';
            if (count > 1 && buffer[count - 1] == '/')
            {
                buffer[count - 1] = '\0';
                --count;
            }
            size_t begin = count;
            size_t slashCount = 0;
            while (slashCount < 2 && begin >= 0)
            {
                --begin;
                if (buffer[begin] == '/')
                {
                    ++slashCount;
                }
            }
            ++begin;
            timezoneName = std::string(buffer + begin);
        }
    }

    // Method 3: tm_zone (always available but faulty)
    if (timezoneName.empty())
    {
        timezoneName = time_info.tm_zone;
    }

    if (buffer != NULL)
    {
        strncpy(buffer, timezoneName.c_str(), buffer_size);
    }

    if (offset != NULL)
    {
        *offset = time_info.tm_gmtoff;
    }

    return timezoneName.size();
}