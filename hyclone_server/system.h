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
#include "server_memory.h"
#include "server_messaging.h"
#include "server_systemnotification.h"
#include "server_usermap.h"

struct Area;
class Process;
class Port;
class Semaphore;
class Thread;
struct haiku_area_info;
struct haiku_fs_info;

class System
{
private:
    bool _isShuttingDown = false;
    int _nextAreaId = 1;
    std::map<int, std::shared_ptr<Process>> _processes;
    std::unordered_map<int, std::shared_ptr<Thread>> _threads;
    std::unordered_map<intptr_t, std::pair<int, int>> _connections;
    IdMap<std::shared_ptr<Port>, int> _ports;
    std::unordered_map<std::string, int> _portNames;
    IdMap<std::shared_ptr<Semaphore>, int> _semaphores;
    IdMap<std::shared_ptr<haiku_fs_info>, int> _fsInfos;
    std::map<int, std::shared_ptr<Area>> _areas;
    std::unordered_map<EntryRef, std::string> _entryRefs;
    std::recursive_mutex _lock;
    AppLoadNotificationService _appLoadNotificationService;
    MemoryService _memoryService;
    MessagingService _messagingService;
    SystemNotificationService _systemNotificationService;
    UserMapService _userMapService;
    System() = default;
    ~System() = default;
public:
    System(const System&) = delete;
    System(System&&) = delete;
    std::weak_ptr<Process> RegisterProcess(int pid, int uid, int gid, int euid, int egid);
    std::weak_ptr<Process> GetProcess(int pid);
    int NextProcessId(int pid) const;
    size_t UnregisterProcess(int pid);

    std::weak_ptr<Thread> RegisterThread(int pid, int tid);
    std::weak_ptr<Thread> GetThread(int tid);
    size_t UnregisterThread(int tid);

    std::pair<int, int> RegisterConnection(intptr_t conn_id, int pid, int tid);
    std::pair<int, int> GetThreadFromConnection(intptr_t conn_id);
    size_t UnregisterConnection(intptr_t conn_id);

    int RegisterPort(std::shared_ptr<Port>&& port);
    std::weak_ptr<Port> GetPort(int port_id);
    int FindPort(const std::string& portName);
    size_t UnregisterPort(int port_id);

    int CreateSemaphore(int pid, int count, const char* name);
    std::weak_ptr<Semaphore> GetSemaphore(int id);
    size_t GetSemaphoreCount() const { return _semaphores.size(); }
    size_t UnregisterSemaphore(int id);

    int RegisterFSInfo(std::shared_ptr<haiku_fs_info>&& info);
    std::weak_ptr<haiku_fs_info> GetFSInfo(int id);
    std::weak_ptr<haiku_fs_info> FindFSInfoByDevId(int devId);
    int NextFSInfoId(int id) const;
    size_t UnregisterFSInfo(int id);

    std::weak_ptr<Area> RegisterArea(const haiku_area_info& info);
    std::weak_ptr<Area> RegisterArea(const std::shared_ptr<Area>& ptr);
    std::weak_ptr<Area> GetArea(int id);
    bool IsValidAreaId(int id) const;
    size_t UnregisterArea(int id);

    int RegisterEntryRef(const EntryRef& ref, const std::string& path);
    int RegisterEntryRef(const EntryRef& ref, std::string&& path);
    int GetEntryRef(const EntryRef& ref, std::string& path) const;

    void Shutdown();
    bool IsShuttingDown() const { return _isShuttingDown; }

    AppLoadNotificationService& GetAppLoadNotificationService() { return _appLoadNotificationService; }
    const AppLoadNotificationService& GetAppLoadNotificationService() const { return _appLoadNotificationService; }

    MemoryService& GetMemoryService() { return _memoryService; }
    const MemoryService& GetMemoryService() const { return _memoryService; }

    MessagingService& GetMessagingService() { return _messagingService; }
    const MessagingService& GetMessagingService() const { return _messagingService; }

    SystemNotificationService& GetSystemNotificationService() { return _systemNotificationService; }
    const SystemNotificationService& GetSystemNotificationService() const { return _systemNotificationService; }

    UserMapService& GetUserMapService() { return _userMapService; }
    const UserMapService& GetUserMapService() const { return _userMapService; }

    std::unique_lock<std::recursive_mutex> Lock() { return std::unique_lock<std::recursive_mutex>(_lock); }

    static System& GetInstance();
};

#endif // __HYCLONE_SYSTEM_H__
