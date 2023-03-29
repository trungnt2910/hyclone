#ifndef __HYCLONE_PROCESS_H__
#define __HYCLONE_PROCESS_H__

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "haiku_area.h"
#include "haiku_image.h"
#include "haiku_team.h"
#include "id_map.h"

class Thread;

class Process
{
private:
    haiku_team_info _info;
    int _pid;
    bool _forkUnlocked;

    std::unordered_map<int, std::shared_ptr<Thread>> _threads;
    IdMap<haiku_extended_image_info, int> _images;
    std::set<int> _areas;
    std::mutex _lock;
    std::unordered_set<int> _owningSemaphores;
    std::unordered_set<int> _owningPorts;
public:
    Process(int pid);
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

    int RegisterArea(int area_id);
    haiku_area_info& GetArea(int area_id);
    int GetAreaIdFor(void* address);
    int NextAreaId(int area_id);
    bool IsValidAreaId(int area_id);
    size_t UnregisterArea(int area_id);

    int GetPid() const { return _pid; }
    haiku_team_info& GetInfo() { return _info; }
    const haiku_team_info& GetInfo() const { return _info; }

    // Copies managed information to child.
    void Fork(Process& child);
    // Checks whether this child process has been unlocked by fork yet.
    bool IsForkUnlocked() const { return _forkUnlocked; }

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
};

#endif // __HYCLONE_PROCESS_H__