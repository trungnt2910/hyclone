#include <cstring>

#include "haiku_errors.h"
#include "process.h"
#include "server_native.h"
#include "server_servercalls.h"
#include "system.h"
#include "thread.h"

Process::Process(int pid)
    : _pid(pid), _forkUnlocked(false)
{
    memset(&_info, 0, sizeof(_info));
    _info.team = pid;
    _info.debugger_nub_thread = -1;
    _info.debugger_nub_port = -1;
}

std::weak_ptr<Thread> Process::RegisterThread(int tid)
{
    auto ptr = std::make_shared<Thread>(_pid, tid);
    _threads[tid] = ptr;
    _info.thread_count = _threads.size();
    return ptr;
}

std::weak_ptr<Thread> Process::RegisterThread(const std::shared_ptr<Thread>& thread)
{
    _threads[thread->GetTid()] = thread;
    _info.thread_count = _threads.size();
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
    _info.thread_count = _threads.size();
    return _threads.size();
}

int Process::RegisterImage(const haiku_extended_image_info& image)
{
    int id = _images.Add(image);
    _images.Get(id).basic_info.id = id;
    _info.image_count = _images.size();
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
    _info.image_count = _images.size();
    return _images.Size();
}

int Process::RegisterArea(int area_id)
{
    _areas.insert(area_id);
    _info.area_count = _areas.size();
    return area_id;
}

haiku_area_info& Process::GetArea(int area_id)
{
    auto& system = System::GetInstance();
    if (!_areas.contains(area_id))
        throw std::runtime_error("Invalid area id");
    // Safe as long as we're holding a lock
    // of the owning process.
    return system.GetArea(area_id);
}

int Process::GetAreaIdFor(void* address)
{
    auto& system = System::GetInstance();

    for (const auto& areaId : _areas)
    {
        const auto& area = system.GetArea(areaId);
        if ((uint8_t*)area.address <= address && address < (uint8_t*)area.address + area.size)
            return area.area;
    }
    return -1;
}

int Process::GetNextAreaIdFor(void* address)
{
    auto& system = System::GetInstance();
    int id = -1;
    uintptr_t min = (uintptr_t)-1;

    for (const auto& areaId : _areas)
    {
        const auto& area = system.GetArea(areaId);
        if ((uintptr_t)area.address > (uintptr_t)address)
        {
            if ((uintptr_t)area.address < min)
            {
                min = (uintptr_t)area.address;
                id = area.area;
            }
        }
    }
    return id;
}

int Process::NextAreaId(int area_id)
{
    auto it = _areas.upper_bound(area_id);
    if (it == _areas.end())
        return -1;
    return *it;
}

bool Process::IsValidAreaId(int area_id)
{
    return _areas.contains(area_id);
}

size_t Process::UnregisterArea(int area_id)
{
    _areas.erase(area_id);
    _info.area_count = _areas.size();
    return _areas.size();
}

void Process::Fork(Process& child)
{
    // No, child has new PID.
    // child._pid = _pid;
    // No, child has its own threads.
    // child._threads = _threads;
    child._info = _info;

    child._images = _images;
    
    // Fork is called in a system lock
    // so we can safely access the areas.
    for (const auto& areaId : _areas)
    {
        auto& system = System::GetInstance();
        haiku_area_info& registeredArea = system.GetArea(
            system.RegisterArea(
                system.GetArea(areaId)));
        registeredArea.team = child._pid;
        child._areas.insert(registeredArea.area);
    }

    // No, semaphores don't seem to be inherited.
    // child._owningSemaphores = _owningSemaphores;

    child._forkUnlocked = true;
}

size_t Process::ReadMemory(void* address, void* buffer, size_t size)
{
    return server_read_process_memory(_pid, address, buffer, size);
}

size_t Process::WriteMemory(void* address, const void* buffer, size_t size)
{
    return server_write_process_memory(_pid, address, buffer, size);
}

intptr_t server_hserver_call_get_team_info(hserver_context& context, int team_id, void* userTeamInfo)
{
    haiku_team_info info;

    if (team_id == B_SYSTEM_TEAM)
    {
        memset(&info, 0, sizeof(info));
        info.debugger_nub_thread = -1;
        info.debugger_nub_port = -1;
        server_fill_team_info(&info);
    }
    else
    {
        std::shared_ptr<Process> process;

        if (team_id == B_CURRENT_TEAM)
        {
            process = context.process;
        }
        else
        {
            auto& system = System::GetInstance();
            auto lock = system.Lock();
            process = system.GetProcess(team_id).lock();
        }

        if (!process)
        {
            return B_BAD_TEAM_ID;
        }

        {
            auto lock = process->Lock();
            info = process->GetInfo();
        }
        server_fill_team_info(&info);
    }

    if (server_write_process_memory(context.pid, userTeamInfo, &info, sizeof(haiku_team_info))
        != sizeof(haiku_team_info))
    {
        return B_BAD_ADDRESS;
    }

    return B_OK;
}

intptr_t server_hserver_call_get_next_team_info(hserver_context& context, int32_t* userCookie, void* userTeamInfo)
{
    int32_t cookie;
    if (server_read_process_memory(context.pid, userCookie, &cookie, sizeof(cookie))
        != sizeof(cookie))
    {
        return B_BAD_ADDRESS;
    }

    if (cookie == 0)
    {
        server_hserver_call_get_team_info(context, B_SYSTEM_TEAM, userTeamInfo);
        cookie = -1;
    }
    else
    {
        {
            auto& system = System::GetInstance();
            auto lock = system.Lock();
            cookie = system.NextProcessId(cookie);
            if (cookie < 0)
            {
                return B_BAD_VALUE;
            }
        }

        status_t status = server_hserver_call_get_team_info(context, cookie, userTeamInfo);

        if (status != B_OK)
        {
            return status;
        }
    }

    if (server_write_process_memory(context.pid, userCookie, &cookie, sizeof(cookie))
        != sizeof(cookie))
    {
        return B_BAD_ADDRESS;
    }

    return B_OK;
}

intptr_t server_hserver_call_register_team_info(hserver_context& context, void* userTeamInfo)
{
    haiku_team_info info;
    if (server_read_process_memory(context.pid, userTeamInfo, &info, sizeof(haiku_team_info))
        != sizeof(haiku_team_info))
    {
        return B_BAD_ADDRESS;
    }

    if (info.team != context.pid)
    {
        return B_NOT_ALLOWED;
    }

    {
        auto lock = context.process->Lock();
        context.process->GetInfo() = info;
    }

    return B_OK;
}