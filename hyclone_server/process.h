#ifndef __HYCLONE_PROCESS_H__
#define __HYCLONE_PROCESS_H__

#include <atomic>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "haiku_area.h"
#include "haiku_image.h"
#include "haiku_team.h"
#include "id_map.h"

class Area;
class Thread;

class Process
{
private:
    haiku_team_info _info;
    int _pid;
    int _uid, _gid, _euid, _egid;
    int _debuggerPid, _debuggerPort, _debuggerWriteLock;
    bool _forkUnlocked;
    std::atomic<bool> _isExecutingExec;
    std::filesystem::path _cwd;

    std::unordered_map<int, std::shared_ptr<Thread>> _threads;
    IdMap<haiku_extended_image_info, int> _images;
    std::map<int, std::shared_ptr<Area>> _areas;
    std::unordered_map<int, std::filesystem::path> _fds;
    std::mutex _lock;
    std::unordered_set<int> _owningSemaphores;
    std::unordered_set<int> _owningPorts;
public:
    Process(int pid, int uid, int gid, int euid, int egid);
    ~Process() = default;

    std::unique_lock<std::mutex> Lock() { return std::unique_lock(_lock); }

    std::weak_ptr<Thread> RegisterThread(int tid);
    std::weak_ptr<Thread> RegisterThread(const std::shared_ptr<Thread>& thread);
    std::weak_ptr<Thread> GetThread(int tid);
    size_t UnregisterThread(int tid);

    int RegisterImage(const haiku_extended_image_info& image);
    haiku_extended_image_info& GetImage(int image_id);
    int NextImageId(int image_id);
    bool IsValidImageId(int image_id);
    size_t UnregisterImage(int image_id);

    std::weak_ptr<Area> RegisterArea(const std::shared_ptr<Area>& area);
    std::weak_ptr<Area> GetArea(int area_id);
    int GetAreaIdFor(void* address);
    int GetNextAreaIdFor(void* address);
    int NextAreaId(int area_id);
    bool IsValidAreaId(int area_id);
    size_t UnregisterArea(int area_id);

    size_t RegisterFd(int fd, const std::filesystem::path& path);
    const std::filesystem::path& GetFd(int fd);
    bool IsValidFd(int fd);
    size_t UnregisterFd(int fd);

    int GetPid() const { return _pid; }
    int GetUid() const { return _uid; }
    int GetGid() const { return _gid; }
    int GetEuid() const { return _euid; }
    int GetEgid() const { return _egid; }
    const std::filesystem::path& GetCwd() const { return _cwd; }
    haiku_team_info& GetInfo() { return _info; }
    const haiku_team_info& GetInfo() const { return _info; }

    void SetCwd(const std::filesystem::path& cwd) { _cwd = cwd; }
    void SetCwd(std::filesystem::path&& cwd) { _cwd = std::move(cwd); }

    int GetDebuggerPid() const { return _debuggerPid; }
    int GetDebuggerPort() const { return _debuggerPort; }
    int GetDebuggerWriteLock() const { return _debuggerWriteLock; }
    void SetDebuggerPid(int pid) { _debuggerPid = pid; }
    void SetDebuggerPort(int port) { _debuggerPort = port; }
    void SetDebuggerWriteLock(int lock) { _debuggerWriteLock = lock; }

    // Copies managed information to child.
    void Fork(Process& child);
    // Checks whether this child process has been unlocked by fork yet.
    bool IsForkUnlocked() const { return _forkUnlocked; }

    void Exec(bool isExec = true);
    bool IsExecutingExec() const { return _isExecutingExec; }
    void WaitForExec();

    const std::unordered_set<int>& GetOwningSemaphores() const { return _owningSemaphores; }
    void AddOwningSemaphore(int id) { _owningSemaphores.insert(id); }
    void RemoveOwningSemaphore(int id) { _owningSemaphores.erase(id); }
    bool IsOwningSemaphore(int id) const { return _owningSemaphores.contains(id); }

    const std::unordered_set<int>& GetOwningPorts() const { return _owningPorts; }
    void AddOwningPort(int id) { _owningPorts.insert(id); }
    void RemoveOwningPort(int id) { _owningPorts.erase(id); }
    bool IsOwningPort(int id) const { return _owningPorts.contains(id); }

    size_t ReadMemory(void* address, void* buffer, size_t size);
    size_t WriteMemory(void* address, const void* buffer, size_t size);

    status_t ReadDirFd(int fd, const void* userBuffer, size_t bufferSize, bool traverseLink, std::filesystem::path& output);
};

#endif // __HYCLONE_PROCESS_H__