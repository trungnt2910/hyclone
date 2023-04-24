#ifndef __SERVER_TIME_H__
#define __SERVER_TIME_H__

#include <chrono>

#include "BeDefs.h"

constexpr auto kMaxTimePoint =
    std::chrono::time_point_cast<std::chrono::microseconds, std::chrono::steady_clock>
        (std::chrono::steady_clock::time_point::max()).time_since_epoch().count();

inline bool server_is_infinite_timeout(bigtime_t timeout, uint32 flags = B_RELATIVE_TIMEOUT)
{
    return timeout == B_INFINITE_TIMEOUT || timeout > kMaxTimePoint
        || ((flags & B_RELATIVE_TIMEOUT) &&
            timeout > (bigtime_t)std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::time_point::max() - std::chrono::steady_clock::now()).count());
}

#endif // __SERVER_TIME_H__