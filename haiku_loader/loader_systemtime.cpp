// This is the correct, cross-platform implementation, but currently
// not supported by GCC (std::chrono::get_tzdb() is not implemented.)
#if 0
#include <chrono>
#include <cstring>

#include "loader_systemtime.h"

size_t loader_system_timezone(char* buffer, size_t size)
{
    const auto& timeZoneDatabase = std::chrono::get_tzdb(); // initialize the time zone database
    const auto& currentZone = timeZoneDatabase.current_zone();
    const auto& currentZoneName = currentZone.name();

    size_t length = currentZoneName.length();

    if (buffer != NULL)
    {
        strncpy(buffer, currentZoneName.c_str(), size);
    }

    return length;
}
#endif