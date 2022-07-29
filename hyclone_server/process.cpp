#include "process.h"
#include "server_native.h"
#include "thread.h"

std::weak_ptr<Thread> Process::RegisterThread(int tid)
{
    auto ptr = std::make_shared<Thread>(_pid, tid);
    _threads[tid] = ptr;
    return ptr;
}

std::weak_ptr<Thread> Process::RegisterThread(const std::shared_ptr<Thread>& thread)
{
    _threads[thread->GetTid()] = thread;
    return thread;
}

std::weak_ptr<Thread> Process::GetThread(int tid)
{
    auto it = _threads.find(tid);
    if (it == _threads.end())
        return std::weak_ptr<Thread>();
    return it->second;
}

size_t Process::UnregisterThread(int tid)
{
    _threads.erase(tid);
    return _threads.size();
}

int Process::RegisterImage(const haiku_extended_image_info& image)
{
    int id = _images.Add(image);
    _images.Get(id).basic_info.id = id;
    return id;
}

haiku_extended_image_info& Process::GetImage(int image_id)
{
    return _images.Get(image_id);
}

int Process::NextImageId(int image_id)
{
    return _images.NextId(image_id);
}

bool Process::IsValidImageId(int image_id)
{
    return _images.IsValidId(image_id);
}

size_t Process::UnregisterImage(int image_id)
{
    _images.Remove(image_id);
    return _images.Size();
}

int Process::RegisterArea(const haiku_area_info& area)
{
    int id = _areas.Add(area);
    auto& newArea = _areas.Get(id);
    newArea.team = _pid;
    newArea.area = id;
    return id;
}

haiku_area_info& Process::GetArea(int area_id)
{
    return _areas.Get(area_id);
}

int Process::GetAreaIdFor(void* address)
{
    for (const auto& area : _areas)
    {
        if ((uint8_t*)area.address <= address && address < (uint8_t*)area.address + area.size)
            return area.area;
    }
    return -1;
}

int Process::NextAreaId(int area_id)
{
    return _areas.NextId(area_id);
}

bool Process::IsValidAreaId(int area_id)
{
    return _areas.IsValidId(area_id);
}

size_t Process::UnregisterArea(int area_id)
{
    _areas.Remove(area_id);
    return _areas.Size();
}

void Process::Fork(Process& child)
{
    // No, child has new PID.
    // child._pid = _pid;
    // No, child has its own threads.
    // child._threads = _threads;

    child._images = _images;
    child._areas = _areas;

    // No, semaphores don't seem to be inherited.
    // child._owningSemaphores = _owningSemaphores;
}

size_t Process::ReadMemory(void* address, void* buffer, size_t size)
{
    return server_read_process_memory(_pid, address, buffer, size);
}

size_t Process::WriteMemory(void* address, const void* buffer, size_t size)
{
    return server_write_process_memory(_pid, address, buffer, size);
}
