#include <cstddef>
#include <cstdint>
#include <cstring>

#include "BeDefs.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "haiku_port.h"
#include "linux_debug.h"

extern "C"
{

port_id MONIKA_EXPORT _kern_create_port(int32 queue_length, const char *name)
{
    // No +1, the server will add the null byte when it reads the memory.
    // name can't be null for ports, we won't check anything here.
    return GET_SERVERCALLS()->create_port(queue_length, name, strlen(name));
}

status_t MONIKA_EXPORT _kern_write_port_etc(port_id port, int32 messageCode, const void *msgBuffer,
    size_t bufferSize, uint32 flags, bigtime_t timeout)
{
    return GET_SERVERCALLS()->write_port_etc(port, messageCode, msgBuffer, bufferSize, flags, timeout);
}

port_id MONIKA_EXPORT _kern_find_port(const char *port_name)
{
    return GET_SERVERCALLS()->find_port(port_name, strlen(port_name));
}

status_t MONIKA_EXPORT _kern_get_port_info(port_id id, struct haiku_port_info *info)
{
    return GET_SERVERCALLS()->get_port_info(id, info);
}

}