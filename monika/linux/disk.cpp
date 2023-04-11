#include <cstddef>

#include "BeDefs.h"
#include "extended_commpage.h"
#include "haiku_errors.h"

extern "C"
{

extern void _moni_debug_output(const char* userString);

partition_id _moni_get_next_disk_device_id(int32* cookie, size_t* neededSize)
{
    // HyClone currently doesn't manage any disk device anyway.
    _moni_debug_output("stub: _kern_get_next_disk_device_id");
    return B_BAD_VALUE;
}

status_t _moni_start_watching(haiku_dev_t device, haiku_ino_t node, uint32 flags,
    port_id port, uint32 token)
{
    _moni_debug_output("stub: _kern_start_watching");
    return HAIKU_POSIX_ENOSYS;
}
status_t _moni_stop_watching(haiku_dev_t device, haiku_ino_t node, port_id port,
    uint32 token)
{
    _moni_debug_output("stub: _kern_stop_watching");
    return HAIKU_POSIX_ENOSYS;
}

haiku_dev_t _moni_next_device(int32 *_cookie)
{
    return GET_SERVERCALLS()->next_device(_cookie);
}

}