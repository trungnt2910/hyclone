#ifndef __HYCLONE_SYSTEM_H__
#define __HYCLONE_SYSTEM_H__

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "entry_ref.h"
#include "id_map.h"
#include "server_apploadnotification.h"

class Process;
class Port;
class Semaphore;
class Thread;
struct haiku_fs_info;

class System
{
private:
    bool _isShuttingDown = false;
    std::unordered_map<int, std::shared_ptr<Process>> _processes;
    std::unordered_map<int, std::shared_ptr<Thread>> _threads;
    std::unordered_map<intptr_t, std::pair<int, int>> _connections;
    IdMap<std::shared_ptr<Port>, int> _ports;
    std::unordered_map<std::string, int> _portNames;
    IdMap<std::shared_ptr<Semaphore>, int> _semaphores;
    IdMap<std::shared_ptr<haiku_fs_info>, int> _fsInfos;
    std::unordered_map<EntryRef, std::string> _entryRefs;
    std::mutex _lock;
    AppLoadNotificationService _appLoadNotificationService;
    System() = default;
    ~System() = default;
public:
    System(const System&) = delete;
    System(System&&) = delete;
    std::weak_ptr<Process> RegisterProcess(int pid);
    std::weak_ptr<Process> GetProcess(int pid);
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

    int RegisterEntryRef(const EntryRef& ref, const std::string& path);
    int RegisterEntryRef(const EntryRef& ref, std::string&& path);
    int GetEntryRef(const EntryRef& ref, std::string& path) const;

    void Shutdown();
    bool IsShuttingDown() const { return _isShuttingDown; }

    AppLoadNotificationService& GetAppLoadNotificationService() { return _appLoadNotificationService; }
    const AppLoadNotificationService& GetAppLoadNotificationService() const { return _appLoadNotificationService; }

    std::unique_lock<std::mutex> Lock() { return std::unique_lock<std::mutex>(_lock); }

    static System& GetInstance();
};

#endif // __HYCLONE_SYSTEM_H__
