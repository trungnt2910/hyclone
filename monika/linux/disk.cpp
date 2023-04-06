#include <cstddef>

#include "BeDefs.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_errors.h"

extern "C"
{

extern void _kern_debug_output(const char* userString);

partition_id MONIKA_EXPORT _kern_get_next_disk_device_id(int32* cookie, size_t* neededSize)
{
    // HyClone currently doesn't manage any disk device anyway.
    _kern_debug_output("stub: _kern_get_next_disk_device_id");
    return B_BAD_VALUE;
}

status_t MONIKA_EXPORT _kern_start_watching(haiku_dev_t device, haiku_ino_t node, uint32 flags,
    port_id port, uint32 token)
{
    _kern_debug_output("stub: _kern_start_watching");
    return HAIKU_POSIX_ENOSYS;
}
status_t MONIKA_EXPORT _kern_stop_watching(haiku_dev_t device, haiku_ino_t node, port_id port,
    uint32 token)
{
    _kern_debug_output("stub: _kern_stop_watching");
    return HAIKU_POSIX_ENOSYS;
}

haiku_dev_t MONIKA_EXPORT _kern_next_device(int32 *_cookie)
{
    return GET_SERVERCALLS()->next_device(_cookie);
}

}