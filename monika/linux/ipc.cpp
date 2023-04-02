#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sys/types.h>

#include "BeDefs.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "haiku_port.h"
#include "linux_debug.h"

#ifdef HYCLONE_DEBUG_PORT
#define DEBUG(...) GET_HOSTCALLS()->printf(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

extern "C"
{

port_id MONIKA_EXPORT _kern_create_port(int32 queue_length, const char *name)
{
    // No +1, the server will add the null byte when it reads the memory.
    // name can't be null for ports, we won't check anything here.
    return GET_SERVERCALLS()->create_port(queue_length, name, strlen(name));
}

size_t MONIKA_EXPORT _kern_read_port_etc(port_id port, int32 *msgCode,
    void *msgBuffer, size_t bufferSize, uint32 flags,
    bigtime_t timeout)
{
    DEBUG("read_port_etc(%d, %p, %p, %d, %d, %lld)\n", port, msgCode, msgBuffer, bufferSize, flags, timeout);
    size_t result = GET_SERVERCALLS()->read_port_etc(port, msgCode, msgBuffer, bufferSize, flags, timeout);
    DEBUG("read_port_etc(%d, %p, %p, %d, %d, %lld) = %d\n", port, msgCode, msgBuffer, bufferSize, flags, timeout, result);
    return result;
}

status_t MONIKA_EXPORT _kern_write_port_etc(port_id port, int32 messageCode, const void *msgBuffer,
    size_t bufferSize, uint32 flags, bigtime_t timeout)
{
    DEBUG("write_port_etc(%d, %d, %p, %d, %d, %lld)\n", port, messageCode, msgBuffer, bufferSize, flags, timeout);
    status_t result = GET_SERVERCALLS()->write_port_etc(port, messageCode, msgBuffer, bufferSize, flags, timeout);
    DEBUG("write_port_etc(%d, %d, %p, %d, %d, %lld) = %d\n", port, messageCode, msgBuffer, bufferSize, flags, timeout, result);
    return result;
}

int32 MONIKA_EXPORT _kern_port_count(port_id port)
{
    DEBUG("port_count(%d)\n", port);
    int32 result = GET_SERVERCALLS()->port_count(port);
    DEBUG("port_count(%d) = %d\n", port, result);
    return result;
}

port_id MONIKA_EXPORT _kern_find_port(const char *port_name)
{
    DEBUG("find_port(%s)\n", port_name);
    port_id result = GET_SERVERCALLS()->find_port(port_name, strlen(port_name));
    DEBUG("find_port(%s) = %d\n", port_name, result);
    return result;
}

status_t MONIKA_EXPORT _kern_get_port_info(port_id id, struct haiku_port_info *info)
{
    DEBUG("get_port_info(%d, %p)\n", id, info);
    status_t result = GET_SERVERCALLS()->get_port_info(id, info);
    DEBUG("get_port_info(%d, %p) = %d\n", id, info, result);
    return result;
}

ssize_t MONIKA_EXPORT _kern_port_buffer_size_etc(port_id port, uint32 flags, bigtime_t timeout)
{
    DEBUG("port_buffer_size_etc(%d, %d, %lld)\n", port, flags, timeout);
    ssize_t result = GET_SERVERCALLS()->port_buffer_size_etc(port, flags, timeout);
    DEBUG("port_buffer_size_etc(%d, %d, %lld) = %d\n", port, flags, timeout, result);
    return result;
}

status_t MONIKA_EXPORT _kern_set_port_owner(port_id port, team_id team)
{
    DEBUG("set_port_owner(%d, %d)\n", port, team);
    status_t result = GET_SERVERCALLS()->set_port_owner(port, team);
    DEBUG("set_port_owner(%d, %d) = %d\n", port, team, result);
    return result;
}

status_t MONIKA_EXPORT _kern_delete_port(port_id id)
{
    DEBUG("delete_port(%d)\n", id);
    status_t result = GET_SERVERCALLS()->delete_port(id);
    DEBUG("delete_port(%d) = %d\n", id, result);
    return result;
}

status_t MONIKA_EXPORT _kern_register_messaging_service(sem_id lockingSem,
    sem_id counterSem)
{
    return GET_SERVERCALLS()->register_messaging_service(lockingSem, counterSem);
}

status_t MONIKA_EXPORT _kern_unregister_messaging_service()
{
    return GET_SERVERCALLS()->unregister_messaging_service();
}

}