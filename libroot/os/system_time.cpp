#include "extended_commpage.h"

// Stub
extern "C" void
__x86_setup_system_time(uint64_t cv, uint64_t cv_nsec)
{

}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int64_t
system_time()
{
    return GET_HOSTCALLS()->system_time();
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int64_t
system_time_nsecs()
{
    return GET_HOSTCALLS()->system_time_nsecs();
}
