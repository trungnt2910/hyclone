#include <cstddef>

#include "BeDefs.h"
#include "export.h"
#include "haiku_errors.h"

extern "C"
{

partition_id MONIKA_EXPORT _kern_get_next_disk_device_id(int32* cookie, size_t* neededSize)
{
    // HyClone currently doesn't manage any disk device anyway.
    return B_BAD_VALUE;
}

status_t MONIKA_EXPORT _kern_start_watching(haiku_dev_t device, haiku_ino_t node, uint32 flags,
    port_id port, uint32 token)
{
    return HAIKU_POSIX_ENOSYS;
}
status_t MONIKA_EXPORT _kern_stop_watching(haiku_dev_t device, haiku_ino_t node, port_id port,
    uint32 token)
{
    return HAIKU_POSIX_ENOSYS;
}

haiku_dev_t MONIKA_EXPORT _kern_next_device(int32 *_cookie)
{
    return B_BAD_VALUE;
}

}