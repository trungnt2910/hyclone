#ifndef __HYCLONE_SYSTEM_H__
#define __HYCLONE_SYSTEM_H__

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "entry_ref.h"
#include "id_map.h"
#include "server_apploadnotification.h"
#include "server_debug.h"
#include "server_memory.h"
#include "server_messaging.h"
#include "server_notificationimpl.h"
#include "server_notifications.h"
#include "server_systemnotification.h"
#include "server_usermap.h"
#include "server_vfs.h"

struct Area;
class Process;
class Port;
class Semaphore;
class Thread;
struct haiku_area_info;
struct haiku_fs_info;

struct Connection
{
    int pid;
    int tid;
    bool isPrimary;

    Connection(int pid_, int tid_, bool isPrimary_ = true)
        : pid(pid_), tid(tid_), isPrimary(isPrimary_) { }
};

class System
{
private:
    bool _isShuttingDown = false;
    int _nextAreaId = 1;
    int _schedulerMode = 0;
    std::map<int, std::shared_ptr<Process>> _processes;
    std::unordered_map<int, std::shared_ptr<Thread>> _threads;
    std::unordered_map<intptr_t, Connection> _connections;
    IdMap<std::shared_ptr<Port>, int> _ports;
    std::unordered_map<std::string, int> _portNames;
    IdMap<std::shared_ptr<Semaphore>, int> _semaphores;
    IdMap<std::shared_ptr<haiku_fs_info>, int> _fsInfos;
    std::map<int, std::shared_ptr<Area>> _areas;
    std::unordered_map<EntryRef, std::string> _entryRefs;
    std::recursive_mutex _lock;
    NotificationManager _notificationManager;
    AppLoadNotificationService _appLoadNotificationService;
    DebugService _debugService;
    MemoryService _memoryService;
    MessagingService _messagingService;
    SystemNotificationService _systemNotificationService;
    TeamNotificationService _teamNotificationService;
    ThreadNotificationService _threadNotificationService;
    UserMapService _userMapService;
    VfsService _vfsService;
    System() = default;
    ~System() = default;
public:
    System(const System&) = delete;
    System(System&&) = delete;

    status_t Init();

    std::weak_ptr<Process> RegisterProcess(int pid, int uid, int gid, int euid, int egid);
    std::weak_ptr<Process> GetProcess(int pid);
    int NextProcessId(int pid) const;
    size_t UnregisterProcess(int pid);

    std::weak_ptr<Thread> RegisterThread(int pid, int tid);
    std::weak_ptr<Thread> GetThread(int tid);
    size_t UnregisterThread(int tid);

    const Connection& RegisterConnection(intptr_t conn_id, const Connection& conn);
    Connection GetThreadFromConnection(intptr_t conn_id);
    size_t UnregisterConnection(intptr_t conn_id);

    int RegisterPort(std::shared_ptr<Port>&& port);
    std::weak_ptr<Port> GetPort(int port_id);
    int FindPort(const std::string& portName);
    size_t UnregisterPort(int port_id);

    int CreateSemaphore(int pid, int count, const char* name);
    std::weak_ptr<Semaphore> GetSemaphore(int id);
    size_t GetSemaphoreCount() const { return _semaphores.size(); }
    size_t UnregisterSemaphore(int id);

    std::weak_ptr<Area> RegisterArea(const haiku_area_info& info);
    std::weak_ptr<Area> RegisterArea(const std::shared_ptr<Area>& ptr);
    std::weak_ptr<Area> GetArea(int id);
    bool IsValidAreaId(int id) const;
    size_t UnregisterArea(int id);

    int GetSchedulerMode() const { return _schedulerMode; }
    void SetSchedulerMode(int mode) { _schedulerMode = mode; }

    void Shutdown();
    bool IsShuttingDown() const { return _isShuttingDown; }

    NotificationManager& GetNotificationManager() { return _notificationManager; }
    const NotificationManager& GetNotificationManager() const { return _notificationManager; }

    AppLoadNotificationService& GetAppLoadNotificationService() { return _appLoadNotificationService; }
    const AppLoadNotificationService& GetAppLoadNotificationService() const { return _appLoadNotificationService; }

    DebugService& GetDebugService() { return _debugService; }
    const DebugService& GetDebugService() const { return _debugService; }

    MemoryService& GetMemoryService() { return _memoryService; }
    const MemoryService& GetMemoryService() const { return _memoryService; }

    MessagingService& GetMessagingService() { return _messagingService; }
    const MessagingService& GetMessagingService() const { return _messagingService; }

    SystemNotificationService& GetSystemNotificationService() { return _systemNotificationService; }
    const SystemNotificationService& GetSystemNotificationService() const { return _systemNotificationService; }

    UserMapService& GetUserMapService() { return _userMapService; }
    const UserMapService& GetUserMapService() const { return _userMapService; }

    VfsService& GetVfsService() { return _vfsService; }
    const VfsService& GetVfsService() const { return _vfsService; }

    std::unique_lock<std::recursive_mutex> Lock() { return std::unique_lock<std::recursive_mutex>(_lock); }

    static System& GetInstance();
};

#endif // __HYCLONE_SYSTEM_H__
