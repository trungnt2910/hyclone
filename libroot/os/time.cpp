#include "BeDefs.h"
#include "extended_commpage.h"

extern "C"
{

void
__arch_init_time(void* data, bool setDefaults)
{
    // Stub.
}


bigtime_t
__arch_get_system_time_offset(void* data)
{
    (void)data;
    return GET_HOSTCALLS()->system_time_offset();
}

}
