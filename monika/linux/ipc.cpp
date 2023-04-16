#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sys/types.h>

#include "BeDefs.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "haiku_port.h"
#include "linux_debug.h"

extern "C"
{

port_id _moni_create_port(int32 queue_length, const char *name)
{
    // No +1, the server will add the null byte when it reads the memory.
    // name can't be null for ports, we won't check anything here.
    return GET_SERVERCALLS()->create_port(queue_length, name, strlen(name));
}

size_t _moni_read_port_etc(port_id port, int32 *msgCode,
    void *msgBuffer, size_t bufferSize, uint32 flags,
    bigtime_t timeout)
{
    return GET_SERVERCALLS()->read_port_etc(port, msgCode, msgBuffer, bufferSize, flags, timeout);
}

status_t _moni_write_port_etc(port_id port, int32 messageCode, const void *msgBuffer,
    size_t bufferSize, uint32 flags, bigtime_t timeout)
{
    return GET_SERVERCALLS()->write_port_etc(port, messageCode, msgBuffer, bufferSize, flags, timeout);
}

int32 _moni_port_count(port_id port)
{
    return GET_SERVERCALLS()->port_count(port);
}

port_id _moni_find_port(const char *port_name)
{
    return GET_SERVERCALLS()->find_port(port_name, strlen(port_name));
}

status_t _moni_get_port_info(port_id id, struct haiku_port_info *info)
{
    return GET_SERVERCALLS()->get_port_info(id, info);
}

status_t _moni_get_next_port_info(team_id team, int32* cookie, struct haiku_port_info *info)
{
    return GET_SERVERCALLS()->get_next_port_info(team, cookie, info);
}

ssize_t _moni_port_buffer_size_etc(port_id port, uint32 flags, bigtime_t timeout)
{
    return GET_SERVERCALLS()->port_buffer_size_etc(port, flags, timeout);
}

status_t _moni_set_port_owner(port_id port, team_id team)
{
    return GET_SERVERCALLS()->set_port_owner(port, team);
}

status_t _moni_close_port(port_id id)
{
    return GET_SERVERCALLS()->close_port(id);
}

status_t _moni_delete_port(port_id id)
{
    return GET_SERVERCALLS()->delete_port(id);
}

status_t _moni_get_port_message_info_etc(port_id port,
    haiku_port_message_info* info, size_t infoSize, uint32 flags,
    bigtime_t timeout)
{
    return GET_SERVERCALLS()->get_port_message_info_etc(port, info, infoSize, flags, timeout);
}

status_t _moni_register_messaging_service(sem_id lockingSem,
    sem_id counterSem)
{
    return GET_SERVERCALLS()->register_messaging_service(lockingSem, counterSem);
}

status_t _moni_unregister_messaging_service()
{
    return GET_SERVERCALLS()->unregister_messaging_service();
}

}