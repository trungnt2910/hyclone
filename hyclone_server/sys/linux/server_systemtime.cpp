#include <time.h>
#include "server_systemtime.h"

// Todo: Make a libsystemtime?
// This is 100% copied from haiku_loader.
int64_t server_system_time()
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);

    return tp.tv_sec * 1000000 + tp.tv_nsec / 1000;
}

int64_t server_system_time_nsecs()
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);

    return tp.tv_sec * 1000000000 + tp.tv_nsec;
}