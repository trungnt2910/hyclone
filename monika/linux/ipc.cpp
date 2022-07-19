#include <cstddef>
#include <cstdint>

#include "BeDefs.h"
#include "export.h"
#include "haiku_errors.h"
#include "haiku_port.h"
#include "linux_debug.h"

extern "C"
{

port_id MONIKA_EXPORT _kern_create_port(int32 queue_length, const char *name)
{
    trace("stub: _kern_create_port");
    return HAIKU_POSIX_ENOSYS;
}

status_t MONIKA_EXPORT _kern_write_port_etc(port_id port, int32 messageCode, const void *msgBuffer,
    size_t bufferSize, uint32 flags, bigtime_t timeout)
{
    trace("stub: _kern_write_port_etc");
    return HAIKU_POSIX_ENOSYS;
}

port_id MONIKA_EXPORT _kern_find_port(const char *port_name)
{
    trace("stub: _kern_find_port");
    return B_NAME_NOT_FOUND;
}

status_t MONIKA_EXPORT _kern_get_port_info(port_id id, struct haiku_port_info *info)
{
    trace("stub: _kern_get_port_info");
    return B_OK;
}

}