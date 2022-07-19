#ifndef __HYCLONE_PROCESS_H__
#define __HYCLONE_PROCESS_H__

#include <memory>
#include <mutex>
#include <unordered_map>

#include "haiku_area.h"
#include "haiku_image.h"
#include "id_map.h"

class Thread;

class Process
{
private:
    int _pid;

    std::unordered_map<int, std::shared_ptr<Thread>> _threads;
    IdMap<haiku_extended_image_info, int> _images;
    IdMap<haiku_area_info, int> _areas;
    std::mutex _lock;
public:
    Process(int pid): _pid(pid) {}
    ~Process() = default;

    std::unique_lock<std::mutex> Lock() { return std::unique_lock(_lock); }

    std::weak_ptr<Thread> RegisterThread(int tid);
    std::weak_ptr<Thread> GetThread(int tid);
    size_t UnregisterThread(int tid);

    int RegisterImage(const haiku_extended_image_info& image);
    haiku_extended_image_info& GetImage(int image_id);
    int NextImageId(int image_id);
    bool IsValidImageId(int image_id);
    size_t UnregisterImage(int image_id);

    int RegisterArea(const haiku_area_info& area);
    haiku_area_info& GetArea(int area_id);
    int GetAreaIdFor(void* address);
    int NextAreaId(int area_id);
    bool IsValidAreaId(int area_id);
    size_t UnregisterArea(int area_id);

    int GetPid() const { return _pid; }

    // Copies managed information to child.
    void Fork(Process& child);

    size_t ReadMemory(void* address, void* buffer, size_t size);
    size_t WriteMemory(void* address, const void* buffer, size_t size);
};

#endif // __HYCLONE_PROCESS_H__