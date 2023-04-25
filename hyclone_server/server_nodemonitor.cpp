#include "io_context.h"
#include "process.h"
#include "server_nodemonitor.h"
#include "server_servercalls.h"
#include "system.h"

const std::string NodeMonitorService::kNodeMonitorServiceName = "node monitor";

typedef std::list<std::shared_ptr<monitor_listener>> MonitorListenerList;

struct node_monitor
{
    // node_monitor* hash_link;
    haiku_dev_t device;
    haiku_ino_t node;
    MonitorListenerList listeners;
};

struct interested_monitor_listener_list
{
    MonitorListenerList::iterator iterator;
    uint32 flags;
};

class UserNodeListener : public UserMessagingListener
{
public:
    UserNodeListener(port_id port, int32 token)
        : UserMessagingListener(System::GetInstance().GetNodeMonitorService()._nodeMonitorSender,
            port, token)
    {
    }

    bool operator==(const NotificationListener& _other) const
    {
        const UserNodeListener* other = dynamic_cast<const UserNodeListener*>(&_other);
        return other != NULL && other->Port() == Port() && other->Token() == Token();
    }
};

// #pragma mark - NodeMonitorService

status_t NodeMonitorService::InitCheck()
{
    return B_OK;
}

/*! Removes the specified node_monitor from the hashtable
    and free it.
    Must be called with monitors lock hold.
*/
void NodeMonitorService::_RemoveMonitor(
    const std::shared_ptr<node_monitor>& monitor, uint32 flags)
{
    if ((flags & B_WATCH_VOLUME) != 0)
        _volumeMonitors.erase(monitor->device);
    else
        _monitors.erase(monitor_hash_key{ monitor->device, monitor->node });
}

//! Helper function for the RemoveListener function.
status_t NodeMonitorService::_RemoveListener(
    const std::shared_ptr<IoContext>& context, haiku_dev_t device,
    haiku_ino_t node, NotificationListener& notificationListener,
    bool isVolumeListener)
{
    std::unique_lock<std::recursive_mutex> _(_recursiveLock);

    // get the monitor for this device/node pair
    std::shared_ptr<node_monitor> monitor = _MonitorFor(device, node, isVolumeListener);
    if (monitor == NULL)
        return B_BAD_VALUE;

    // see if it has the listener we are looking for
    std::shared_ptr<monitor_listener> listener = _MonitorListenerFor(monitor,
        notificationListener);
    if (listener == NULL)
        return B_BAD_VALUE;

    _RemoveListener(context, listener);

    return B_OK;
}

/*! Removes the specified monitor_listener from all lists
    and free it.
    Must be called with monitors lock hold.
*/
void NodeMonitorService::_RemoveListener(
    const std::shared_ptr<IoContext>& context,
    const std::shared_ptr<monitor_listener>& listener)
{
    uint32 flags = listener->flags;
    const std::shared_ptr<node_monitor>& monitor = listener->monitor;

    // remove it from the listener and I/O context lists
    monitor->listeners.erase(listener->monitor_link);
    context->RemoveMonitor(listener);

    if (dynamic_cast<UserNodeListener*>(listener->listener) != NULL)
    {
        // This is a listener we copied ourselves in UpdateUserListener(),
        // so we have to delete it here.
        delete listener->listener;
    }

    if (monitor->listeners.empty())
        _RemoveMonitor(monitor, flags);
}

/*! Returns the monitor that matches the specified device/node pair.
    Must be called with monitors lock hold.
*/
std::shared_ptr<node_monitor> NodeMonitorService::_MonitorFor(
    haiku_dev_t device, haiku_ino_t node, bool isVolumeListener)
{
    if (isVolumeListener)
    {
        return _volumeMonitors.contains(device) ? _volumeMonitors[device] :
            std::shared_ptr<node_monitor>();
    }

    struct monitor_hash_key key;
    key.device = device;
    key.node = node;

    return _monitors.contains(key) ? _monitors[key] :
        std::shared_ptr<node_monitor>();
}

/*! Returns the monitor that matches the specified device/node pair.
    If the monitor does not exist yet, it will be created.
    Must be called with monitors lock hold.
*/
status_t NodeMonitorService::_GetMonitor(const std::shared_ptr<IoContext>& context,
    haiku_dev_t device, haiku_ino_t node,
    bool addIfNecessary, std::shared_ptr<node_monitor>* _monitor, bool isVolumeListener)
{
    std::shared_ptr<node_monitor> monitor = _MonitorFor(device, node, isVolumeListener);
    if (monitor)
    {
        *_monitor = monitor;
        return B_OK;
    }
    if (!addIfNecessary)
        return B_BAD_VALUE;

    // check if this team is allowed to have more listeners
    if (context->NumMonitors() >= context->MaxMonitors())
    {
        // the BeBook says to return B_NO_MEMORY in this case, but
        // we should have another one.
        return B_NO_MEMORY;
    }

    // create new monitor
    monitor = std::make_shared<node_monitor>();
    if (!monitor)
        return B_NO_MEMORY;

    // initialize monitor
    monitor->device = device;
    monitor->node = node;

    if (isVolumeListener)
        _volumeMonitors.emplace(monitor->device, monitor);
    else
        _monitors.emplace(monitor_hash_key{ monitor->device, monitor->node }, monitor);

    *_monitor = monitor;
    return B_OK;
}

/*! Returns the listener that matches the specified port/token pair.
    Must be called with monitors lock hold.
*/
std::shared_ptr<monitor_listener> NodeMonitorService::_MonitorListenerFor(
    const std::shared_ptr<node_monitor>& monitor,
    NotificationListener& notificationListener)
{
    for (auto& listener : monitor->listeners)
    {
        // does this listener match?
        if (*listener->listener == notificationListener)
            return listener;
    }

    return std::shared_ptr<monitor_listener>();
}

status_t NodeMonitorService::_AddMonitorListener(
    const std::shared_ptr<IoContext>& context,
    const std::shared_ptr<node_monitor>& monitor, uint32 flags,
    NotificationListener& notificationListener)
{
    auto listener = std::make_shared<monitor_listener>();
    if (listener == NULL)
    {
        // no memory for the listener, so remove the monitor as well if needed
        if (monitor->listeners.empty())
            _RemoveMonitor(monitor, flags);

        return B_NO_MEMORY;
    }

    // initialize listener, and add it to the lists
    listener->listener = &notificationListener;
    listener->flags = flags;
    listener->monitor = monitor;

    monitor->listeners.push_back(listener);
    listener->monitor_link = --monitor->listeners.end();
    context->AddMonitor(listener);

    return B_OK;
}

status_t NodeMonitorService::AddListener(const std::shared_ptr<IoContext>& context,
    haiku_dev_t device, haiku_ino_t node,
    uint32 flags, NotificationListener& notificationListener)
{
    std::unique_lock<std::recursive_mutex> _(_recursiveLock);

    std::shared_ptr<node_monitor> monitor;
    status_t status = _GetMonitor(context, device, node, true, &monitor,
        (flags & B_WATCH_VOLUME) != 0);
    if (status < B_OK)
        return status;

    // add listener

    return _AddMonitorListener(context, monitor, flags, notificationListener);
}


status_t NodeMonitorService::_UpdateListener(
    const std::shared_ptr<IoContext>& context,
    haiku_dev_t device, haiku_ino_t node, uint32 flags,
    bool addFlags, NotificationListener& notificationListener)
{
    std::unique_lock<std::recursive_mutex> _(_recursiveLock);

    std::shared_ptr<node_monitor> monitor;
    status_t status = _GetMonitor(context, device, node, false, &monitor,
                                  (flags & B_WATCH_VOLUME) != 0);
    if (status < B_OK)
        return status;

    for (auto& listener : monitor->listeners)
    {
        if (*listener->listener == notificationListener)
        {
            if (addFlags)
                listener->flags |= flags;
            else
                listener->flags = flags;
            return B_OK;
        }
    }

    return B_BAD_VALUE;
}

status_t NodeMonitorService::RemoveListeners(
    const std::shared_ptr<IoContext>& context)
{
    std::unique_lock<std::recursive_mutex> locker(_recursiveLock);

    while (!context->GetMonitors().empty())
    {
        // the _RemoveListener() method will also free the node_monitor
        // if it doesn't have any listeners attached anymore
        _RemoveListener(context, context->GetMonitors().front());
    }

    return B_OK;
}

status_t NodeMonitorService::AddListener(const KMessage* eventSpecifier,
    NotificationListener& listener)
{
    if (eventSpecifier == NULL)
        return B_BAD_VALUE;

    dev_t device = eventSpecifier->GetInt32("device", -1);
    ino_t node = eventSpecifier->GetInt64("node", -1);
    uint32 flags = eventSpecifier->GetInt32("flags", 0);

    return AddListener(gCurrentContext->process->GetIoContext(), device, node, flags, listener);
}

status_t NodeMonitorService::UpdateListener(const KMessage* eventSpecifier,
    NotificationListener& listener)
{
    if (eventSpecifier == NULL)
        return B_BAD_VALUE;

    dev_t device = eventSpecifier->GetInt32("device", -1);
    ino_t node = eventSpecifier->GetInt64("node", -1);
    uint32 flags = eventSpecifier->GetInt32("flags", 0);
    bool addFlags = eventSpecifier->GetBool("add flags", false);

    return _UpdateListener(gCurrentContext->process->GetIoContext(), device, node, flags, addFlags, listener);
}

status_t NodeMonitorService::RemoveListener(const KMessage* eventSpecifier,
    NotificationListener& listener)
{
    if (eventSpecifier == NULL)
        return B_BAD_VALUE;

    dev_t device = eventSpecifier->GetInt32("device", -1);
    ino_t node = eventSpecifier->GetInt64("node", -1);

    return RemoveListener(gCurrentContext->process->GetIoContext(), device, node, listener);
}

status_t NodeMonitorService::RemoveListener(
    const std::shared_ptr<IoContext>& context, haiku_dev_t device,
    haiku_ino_t node, NotificationListener& notificationListener)
{
    std::unique_lock<std::recursive_mutex> _(_recursiveLock);

    if (_RemoveListener(context, device, node, notificationListener, false) == B_OK)
        return B_OK;

    return _RemoveListener(context, device, node, notificationListener, true);
}

inline status_t NodeMonitorService::RemoveUserListeners(
    const std::shared_ptr<IoContext>& context,
    port_id port, uint32 token)
{
    UserNodeListener userListener(port, token);
    std::shared_ptr<monitor_listener> listener;
    int32 count = 0;

    std::unique_lock<std::recursive_mutex> _(_recursiveLock);

    for (auto it = context->GetMonitors().begin(); it != context->GetMonitors().end();)
    {
        const std::shared_ptr<monitor_listener>& removeListener = *it;
        auto nextIt = std::next(it);

        if (*listener->listener != userListener)
        {
            it = nextIt;
            continue;
        }

        _RemoveListener(context, removeListener);
        ++count;

        it = nextIt;
    }

    return count > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}

status_t NodeMonitorService::UpdateUserListener(const std::shared_ptr<IoContext>& context,
    haiku_dev_t device, haiku_ino_t node, uint32 flags, UserNodeListener& userListener)
{
    std::unique_lock<std::recursive_mutex> _(_recursiveLock);

    std::shared_ptr<node_monitor> monitor;
    status_t status = _GetMonitor(context, device, node, true, &monitor,
                                  (flags & B_WATCH_VOLUME) != 0);
    if (status < B_OK)
        return status;

    for (auto& listener : monitor->listeners)
    {
        if (*listener->listener == userListener)
        {
            listener->flags = flags;
            return B_OK;
        }
    }

    UserNodeListener* copiedListener = new (std::nothrow) UserNodeListener(
        userListener);
    if (!copiedListener)
    {
        if (monitor->listeners.empty())
            _RemoveMonitor(monitor, flags);
        return B_NO_MEMORY;
    }

    status = _AddMonitorListener(context, monitor, flags, *copiedListener);
    if (status != B_OK)
        delete copiedListener;

    return status;
}

// #pragma mark - Servercalls

intptr_t server_hserver_call_stop_notifying(hserver_context& context,
    port_id port, unsigned int token)
{
    std::shared_ptr<IoContext> ioContext = context.process->GetIoContext();

    return System::GetInstance().GetNodeMonitorService().RemoveUserListeners(
        ioContext, port, token);
}

intptr_t server_hserver_call_start_watching(hserver_context& context,
    unsigned long long device, unsigned long long node, unsigned int flags,
    int port, unsigned int token)
{
    std::shared_ptr<IoContext> ioContext = context.process->GetIoContext();

    UserNodeListener listener(port, token);

    return System::GetInstance().GetNodeMonitorService().UpdateUserListener(
        ioContext, device, node, flags, listener);
}

intptr_t server_hserver_call_stop_watching(hserver_context& context,
    unsigned long long device, unsigned long long node, int port,
    unsigned int token)
{
    std::shared_ptr<IoContext> ioContext = context.process->GetIoContext();

    UserNodeListener listener(port, token);
    return System::GetInstance().GetNodeMonitorService().RemoveListener(
        ioContext, device, node, listener);
}