#ifndef __LOADER_SYSTEMTIME_H__
#define __LOADER_SYSTEMTIME_H__

#include <cstddef>
#include <cstdint>

// Returns the number of microseconds since boot.
int64_t loader_system_time();
// Returns the number of nanoseconds since boot.
int64_t loader_system_time_nsecs();

// Gets the system time zone name.
size_t loader_system_timezone(int* offset, char* buffer, size_t buffer_size);

#endif // __LOADER_SYSTEMTIME_H__