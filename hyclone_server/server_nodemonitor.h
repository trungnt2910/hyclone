#ifndef __SERVER_NODEMONITOR_H__
#define __SERVER_NODEMONITOR_H__

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "BeDefs.h"
#include "server_notifications.h"

struct node_monitor;
struct interested_monitor_listener_list;

class KMessage;

class IoContext;

class UserNodeListener;

struct monitor_listener
{
    std::list<std::shared_ptr<monitor_listener>>::iterator context_link;
    std::list<std::shared_ptr<monitor_listener>>::iterator monitor_link;
    NotificationListener* listener;
    uint32 flags;
    std::shared_ptr<node_monitor> monitor;
};

class NodeMonitorService : public NotificationService
{
    friend class UserNodeListener;
private:
    void _RemoveMonitor(const std::shared_ptr<node_monitor>& monitor, uint32 flags);
    status_t _RemoveListener(const std::shared_ptr<IoContext>& context,
        haiku_dev_t device, haiku_ino_t node,
        NotificationListener& notificationListener, bool isVolumeListener);
    void _RemoveListener(const std::shared_ptr<IoContext>& context,
        const std::shared_ptr<monitor_listener>& listener);
    std::shared_ptr<node_monitor> _MonitorFor(haiku_dev_t device, haiku_ino_t node,
        bool isVolumeListener);
    status_t _GetMonitor(const std::shared_ptr<IoContext>& context,
        haiku_dev_t device, haiku_ino_t node,
        bool addIfNecessary, std::shared_ptr<node_monitor>* _monitor,
        bool isVolumeListener);
    std::shared_ptr<monitor_listener> _MonitorListenerFor(
        const std::shared_ptr<node_monitor>& monitor,
        NotificationListener& notificationListener);
    status_t _AddMonitorListener(const std::shared_ptr<IoContext>& context,
        const std::shared_ptr<node_monitor>& monitor, uint32 flags,
        NotificationListener& notificationListener);
    status_t _UpdateListener(const std::shared_ptr<IoContext>& context,
        haiku_dev_t device, haiku_ino_t node,
        uint32 flags, bool addFlags,
        NotificationListener& notificationListener);
    void _GetInterestedMonitorListeners(haiku_dev_t device, haiku_ino_t node,
        uint32 flags, interested_monitor_listener_list* interestedListeners,
        int32& interestedListenerCount);
    void _GetInterestedVolumeListeners(haiku_dev_t device, uint32 flags,
        interested_monitor_listener_list* interestedListeners,
        int32& interestedListenerCount);
    status_t _SendNotificationMessage(KMessage& message,
        interested_monitor_listener_list* interestedListeners,
        int32 interestedListenerCount);
    void _ResolveMountPoint(haiku_dev_t device, haiku_ino_t directory,
        haiku_dev_t& parentDevice, haiku_ino_t& parentDirectory);

    struct monitor_hash_key
    {
        haiku_dev_t device;
        haiku_ino_t node;

        bool operator==(const monitor_hash_key& other) const = default;
    };

    struct HashDefinition
    {
        uint32 _Hash(haiku_dev_t device, haiku_ino_t node) const
        {
            return ((uint32)(node >> 32) + (uint32)node) ^ (uint32)device;
        }

        size_t operator()(const monitor_hash_key& key) const
        {
            return _Hash(key.device, key.node);
        }
    };

    std::unordered_map<monitor_hash_key, std::shared_ptr<node_monitor>, HashDefinition> _monitors;
    std::unordered_map<haiku_dev_t, std::shared_ptr<node_monitor>> _volumeMonitors;
    std::recursive_mutex _recursiveLock;

    UserMessagingMessageSender _nodeMonitorSender;

    static constexpr std::string kNodeMonitorServiceName = "node monitor";
public:
    NodeMonitorService() = default;
    virtual ~NodeMonitorService() override = default;

    status_t InitCheck();

    status_t NotifyEntryCreatedOrRemoved(int32 opcode, haiku_dev_t device,
        haiku_ino_t directory, const std::string& name, haiku_ino_t node);
    status_t NotifyEntryMoved(haiku_dev_t device, haiku_ino_t fromDirectory,
        const std::string& fromName, haiku_ino_t toDirectory, const std::string& toName,
        haiku_ino_t node);
    status_t NotifyStatChanged(haiku_dev_t device, haiku_ino_t directory, haiku_ino_t node,
        uint32 statFields);
    status_t NotifyAttributeChanged(haiku_dev_t device, haiku_ino_t directory,
        haiku_ino_t node, const std::string& attribute, int32 cause);
    status_t NotifyUnmount(haiku_dev_t device);
    status_t NotifyMount(haiku_dev_t device, haiku_dev_t parentDevice,
        haiku_ino_t parentDirectory);

    status_t RemoveListeners(const std::shared_ptr<IoContext>& context);

    status_t AddListener(const KMessage* eventSpecifier,
        NotificationListener& listener);
    status_t UpdateListener(const KMessage* eventSpecifier,
        NotificationListener& listener);
    status_t RemoveListener(const KMessage* eventSpecifier,
        NotificationListener& listener);

    status_t AddListener(const std::shared_ptr<IoContext>& context, haiku_dev_t device, haiku_ino_t node,
        uint32 flags, NotificationListener& notificationListener);
    status_t RemoveListener(const std::shared_ptr<IoContext>& context, haiku_dev_t device, haiku_ino_t node,
        NotificationListener& notificationListener);

    status_t RemoveUserListeners(const std::shared_ptr<IoContext>& context,
        port_id port, uint32 token);
    status_t UpdateUserListener(const std::shared_ptr<IoContext>& context, haiku_dev_t device,
        haiku_ino_t node, uint32 flags, UserNodeListener& userListener);

    virtual const std::string& Name() override { return kNodeMonitorServiceName; }
};

enum
{
    B_WATCH_VOLUME = 0xF000
};

#endif // __SERVER_NODEMONITOR_H__