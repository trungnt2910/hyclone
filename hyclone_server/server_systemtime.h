#ifndef __HYCLONE_SYSTEMTIME_H__
#define __HYCLONE_SYSTEMTIME_H__

#include <cstdint>

// Returns the number of microseconds since boot.
int64_t server_system_time();
// Returns the number of nanoseconds since boot.
int64_t server_system_time_nsecs();

#endif // __LOADER_SYSTEMTIME_H__