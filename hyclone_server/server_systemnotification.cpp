#include <vector>

#include "haiku_errors.h"
#include "port.h"
#include "server_servercalls.h"
#include "server_systemnotification.h"
#include "system.h"

status_t SystemNotificationService::StartListening(int32 object, uint32 flags, std::shared_ptr<Port> port, int32 token)
{
    if (object < 0 && object != -1)
    {
        return B_BAD_VALUE;
    }

    if ((flags & B_WATCH_SYSTEM_ALL) == 0
        || (flags & ~(uint32)B_WATCH_SYSTEM_ALL) != 0)
    {
        return B_BAD_VALUE;
    }

    if (!_listeners.contains(object))
    {
        _listeners[object] = std::list<Listener>();
    }

    auto& listeners = _listeners[object];
    auto listener = FindListener(object, port, token);

    if (listener != listeners.end())
    {
        // Just add the new flags
        listener->flags |= flags;
        return B_OK;
    }

    _listeners[object].push_front({ port, flags, token });
    return B_OK;
}

status_t SystemNotificationService::StopListening(int32 object, uint32_t flags, std::shared_ptr<Port> port, int32 token)
{
    if (object < 0 && object != -1)
    {
        return B_BAD_VALUE;
    }

    if ((flags & B_WATCH_SYSTEM_ALL) == 0
        || (flags & ~(uint32)B_WATCH_SYSTEM_ALL) != 0)
    {
        return B_BAD_VALUE;
    }

    if (!_listeners.contains(object))
    {
        return B_ENTRY_NOT_FOUND;
    }

    auto& listeners = _listeners[object];
    auto listener = FindListener(object, port, token);

    if (listener == listeners.end())
    {
        return B_ENTRY_NOT_FOUND;
    }

    listener->flags &= ~flags;

    if (listener->flags != 0)
    {
        return B_OK;
    }

    listeners.erase(listener);

    if (listeners.empty())
    {
        _listeners.erase(object);
    }

    return B_OK;
}

std::list<SystemNotificationService::Listener>::iterator
SystemNotificationService::FindListener(int32 object, std::shared_ptr<Port> port, int32 token)
{
    auto& listeners = _listeners[object];

    for (auto it = listeners.begin(); it != listeners.end(); )
    {
        auto itPort = it->port.lock();
        if (!itPort)
        {
            auto prevIt = it;
            ++it;
            listeners.erase(prevIt);
        }

        if (itPort == port && it->token == token)
            return it;

        ++it;
    }

    return listeners.end();
}

intptr_t server_hserver_call_start_watching_system(hserver_context& context,
    int object, unsigned int flags, int port, int token)
{
    auto& system = System::GetInstance();
    auto& notiService = system.GetSystemNotificationService();

    std::shared_ptr<Port> portPtr;

    {
        auto lock = system.Lock();
        portPtr = system.GetPort(port).lock();
    }

    if (!portPtr)
    {
        return B_BAD_PORT_ID;
    }

    return notiService.StartListening(object, flags, portPtr, token);
}

intptr_t server_hserver_call_stop_watching_system(hserver_context& context,
    int object, unsigned int flags, int port, int token)
{
    auto& system = System::GetInstance();
    auto& notiService = system.GetSystemNotificationService();

    std::shared_ptr<Port> portPtr;

    {
        auto lock = system.Lock();
        portPtr = system.GetPort(port).lock();
    }

    if (!portPtr)
    {
        return B_BAD_PORT_ID;
    }

    return notiService.StopListening(object, flags, portPtr, token);
}