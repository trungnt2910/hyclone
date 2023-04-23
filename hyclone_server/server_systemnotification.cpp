#include "haiku_errors.h"
#include "kmessage.h"
#include "port.h"
#include "process.h"
#include "server_servercalls.h"
#include "server_systemnotification.h"
#include "system.h"

status_t SystemNotificationService::Init()
{
    auto& notificationManager = System::GetInstance().GetNotificationManager();

    status_t error;

    error = notificationManager.AddListener("teams",
        TEAM_ADDED | TEAM_REMOVED | TEAM_EXEC, *this);

    if (error != B_OK)
    {
        return error;
    }

    error = notificationManager.AddListener("threads",
        THREAD_ADDED | THREAD_REMOVED | TEAM_EXEC, *this);

    if (error != B_OK)
    {
        return error;
    }

    return B_OK;
}

status_t SystemNotificationService::StartListening(int32 object, uint32 flags,
    port_id port, int32 token)
{
    // check the parameters
    if ((object < 0 && object != -1) || port < 0)
    {
        return B_BAD_VALUE;
    }

    if ((flags & B_WATCH_SYSTEM_ALL) == 0
        || (flags & ~(uint32)B_WATCH_SYSTEM_ALL) != 0)
    {
        return B_BAD_VALUE;
    }

    auto locker = std::unique_lock(_lock);

    // maybe the listener already exists
    ListenerList* listenerList;
    auto listener = _FindListener(object, port, token, listenerList);
    if (listener)
    {
        // just add the new flags
        listener->flags |= flags;
        return B_OK;
    }

    // create a new listener
    listener = std::make_shared<Listener>();
    if (!listener)
    {
        return B_NO_MEMORY;
    }

    listener->port = port;
    listener->token = token;
    listener->flags = flags;

    // if there's no list yet, create a new list
    if (listenerList == NULL)
    {
        listenerList = &_teamListeners[object];
    }

    listenerList->listeners.push_back(listener);
    listener->list = listenerList;
    listener->listLink = --listenerList->listeners.end();

    gCurrentContext->process->AddData(listener);

    return B_OK;
}

status_t SystemNotificationService::StopListening(int32 object, uint32_t flags,
    port_id port, int32 token)
{
    auto locker = std::unique_lock(_lock);

    // find the listener
    ListenerList* listenerList;
    auto listener = _FindListener(object, port, token, listenerList);
    if (!listener)
    {
        return B_ENTRY_NOT_FOUND;
    }

    // clear the given flags
    listener->flags &= ~flags;

    if (listener->flags != 0)
    {
        return B_OK;
    }

    gCurrentContext->process->RemoveData(listener);
    _RemoveListener(listener);

    return B_OK;
}

void SystemNotificationService::EventOccurred(NotificationService& service,
    const KMessage* event)
{
    auto locker = std::unique_lock(_lock);

    int32 eventCode;
    int32 teamID = 0;
    if (event->FindInt32("event", &eventCode) != B_OK
        || event->FindInt32("team", &teamID) != B_OK)
    {
        return;
    }

    int32 object;
    uint32 opcode;
    uint32 flags;

    // translate the event
    if (event->What() == TEAM_MONITOR)
    {
        switch (eventCode)
        {
        case TEAM_ADDED:
            opcode = B_TEAM_CREATED;
            flags = B_WATCH_SYSTEM_TEAM_CREATION;
            break;
        case TEAM_REMOVED:
            opcode = B_TEAM_DELETED;
            flags = B_WATCH_SYSTEM_TEAM_DELETION;
            break;
        case TEAM_EXEC:
            opcode = B_TEAM_EXEC;
            flags = B_WATCH_SYSTEM_TEAM_CREATION | B_WATCH_SYSTEM_TEAM_DELETION;
            break;
        default:
            return;
        }

        object = teamID;
    }
    else if (event->What() == THREAD_MONITOR)
    {
        if (event->FindInt32("thread", &object) != B_OK)
            return;

        switch (eventCode)
        {
        case THREAD_ADDED:
            opcode = B_THREAD_CREATED;
            flags = B_WATCH_SYSTEM_THREAD_CREATION;
            break;
        case THREAD_REMOVED:
            opcode = B_THREAD_DELETED;
            flags = B_WATCH_SYSTEM_THREAD_DELETION;
            break;
        case THREAD_NAME_CHANGED:
            opcode = B_THREAD_NAME_CHANGED;
            flags = B_WATCH_SYSTEM_THREAD_PROPERTIES;
            break;
        default:
            return;
        }
    }
    else
        return;

    // find matching listeners
    messaging_target targets[kMaxMessagingTargetCount];
    int32 targetCount = 0;

    _AddTargets(&_teamListeners[teamID], flags, targets,
                targetCount, object, opcode);
    _AddTargets(&_teamListeners[-1], flags, targets, targetCount,
                object, opcode);

    // send the message
    if (targetCount > 0)
    {
        _SendMessage(targets, targetCount, object, opcode);
    }
}

void SystemNotificationService::_AddTargets(ListenerList* listenerList, uint32 flags,
    messaging_target* targets, int32& targetCount, int32 object,
    uint32 opcode)
{
    if (listenerList == NULL)
    {
        return;
    }

    for (auto& listener : listenerList->listeners)
    {
        if ((listener->flags & flags) == 0)
        {
            continue;
        }

        // array is full -- need to flush it first
        if (targetCount == kMaxMessagingTargetCount)
        {
            _SendMessage(targets, targetCount, object, opcode);
            targetCount = 0;
        }

        // add the listener
        targets[targetCount].port = listener->port;
        targets[targetCount++].token = listener->token;
    }
}

void SystemNotificationService::_SendMessage(messaging_target* targets,
    int32 targetCount, int32 object, uint32 opcode)
{
    // prepare the message
    char buffer[128];
    KMessage message;
    message.SetTo(buffer, sizeof(buffer), B_SYSTEM_OBJECT_UPDATE);
    message.AddInt32("opcode", opcode);
    if (opcode < B_THREAD_CREATED)
    {
        message.AddInt32("team", object);
    }
    else
    {
        message.AddInt32("thread", object);
    }

    // send it
    {
        auto& messagingService = System::GetInstance().GetMessagingService();
        auto lock = messagingService.Lock();
        messagingService.SendMessage(message.Buffer(), message.ContentSize(),
            targets, targetCount);
    }
}

std::shared_ptr<SystemNotificationService::Listener> SystemNotificationService::_FindListener(
    int32 object, port_id port, int32 token, ListenerList*& listenerList)
{
    if (!_teamListeners.contains(object))
    {
        listenerList = NULL;
        return std::shared_ptr<Listener>();
    }

    listenerList = &_teamListeners[object];

    auto it = std::find_if(listenerList->listeners.begin(),
        listenerList->listeners.end(),
        [port, token](const std::shared_ptr<Listener>& listener)
        {
            return listener->port == port && listener->token == token;
        });

    if (it == listenerList->listeners.end())
    {
        return std::shared_ptr<Listener>();
    }

    return *it;
}

void SystemNotificationService::_RemoveObsoleteListener(
    const std::shared_ptr<Listener>& listener)
{
    auto locker = std::unique_lock(_lock);
    _RemoveListener(listener);
}

void SystemNotificationService::_RemoveListener(
    const std::shared_ptr<Listener>& listener)
{
    // no flags anymore -- remove the listener
    auto listenerList = listener->list;
    listenerList->listeners.erase(listener->listLink);

    if (listenerList->listeners.empty())
    {
        _teamListeners.erase(listenerList->object);
    }
}

void SystemNotificationService::Listener::OwnerDeleted(AssociatedDataOwner* owner)
{
    System::GetInstance().GetSystemNotificationService()._RemoveObsoleteListener(
        std::static_pointer_cast<Listener>(this->shared_from_this()));
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

    return notiService.StartListening(object, flags, port, token);
}

intptr_t server_hserver_call_stop_watching_system(hserver_context& context,
    int object, unsigned int flags, int port, int token)
{
    auto& system = System::GetInstance();
    auto& notiService = system.GetSystemNotificationService();

    return notiService.StopListening(object, flags, port, token);
}