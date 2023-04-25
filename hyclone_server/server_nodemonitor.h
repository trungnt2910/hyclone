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

    static const std::string kNodeMonitorServiceName;
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

// Flags for the watch_node() call.

// Note that B_WATCH_MOUNT is NOT included in B_WATCH_ALL.
// You may prefer to use BVolumeRoster for volume watching.

enum
{
    B_STOP_WATCHING = 0x0000,
    B_WATCH_NAME = 0x0001,
    B_WATCH_STAT = 0x0002,
    B_WATCH_ATTR = 0x0004,
    B_WATCH_DIRECTORY = 0x0008,
    B_WATCH_ALL = 0x000f,

    B_WATCH_MOUNT = 0x0010,
    B_WATCH_INTERIM_STAT = 0x0020,
    B_WATCH_CHILDREN = 0x0040
};

// The "opcode" field of the B_NODE_MONITOR notification message you get.

// The presence and meaning of the other fields in that message specifying what
// exactly caused the notification depend on this value.

#define B_ENTRY_CREATED 1
#define B_ENTRY_REMOVED 2
#define B_ENTRY_MOVED 3
#define B_STAT_CHANGED 4
#define B_ATTR_CHANGED 5
#define B_DEVICE_MOUNTED 6
#define B_DEVICE_UNMOUNTED 7

// More specific info in the "cause" field of B_ATTR_CHANGED notification
// messages. (Haiku only)

#define B_ATTR_CREATED 1
#define B_ATTR_REMOVED 2
//      B_ATTR_CHANGED is reused

enum
{
    B_NODE_MONITOR = 'NDMN',
};

#endif // __SERVER_NODEMONITOR_H__