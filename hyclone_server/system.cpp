#include <cstddef>
#include <cstring>
#include <iostream>

#include "entry_ref.h"
#include "haiku_area.h"
#include "haiku_errors.h"
#include "haiku_fs_info.h"
#include "hsemaphore.h"
#include "port.h"
#include "process.h"
#include "server_native.h"
#include "server_servercalls.h"
#include "server_workers.h"
#include "system.h"
#include "thread.h"

System& System::GetInstance()
{
    static System instance;
    return instance;
}

std::weak_ptr<Process> System::RegisterProcess(int pid)
{
    auto ptr = std::make_shared<Process>(pid);
    _processes[pid] = ptr;
    return ptr;
}

std::weak_ptr<Process> System::GetProcess(int pid)
{
    auto it = _processes.find(pid);
    if (it == _processes.end())
        return std::weak_ptr<Process>();
    return it->second;
}

int System::NextProcessId(int pid) const
{
    auto it = _processes.upper_bound(pid);
    if (it == _processes.end())
        return -1;
    return it->first;
}

size_t System::UnregisterProcess(int pid)
{
    _processes.erase(pid);
    return _processes.size();
}

std::weak_ptr<Thread> System::RegisterThread(int pid, int tid)
{
    auto ptr = std::make_shared<Thread>(pid, tid);
    _threads[tid] = ptr;
    return ptr;
}

std::weak_ptr<Thread> System::GetThread(int tid)
{
    auto it = _threads.find(tid);
    if (it == _threads.end())
        return std::weak_ptr<Thread>();
    return it->second;
}

size_t System::UnregisterThread(int tid)
{
    _threads.erase(tid);
    return _threads.size();
}

std::pair<int, int> System::RegisterConnection(intptr_t conn_id, int pid, int tid)
{
    _connections[conn_id] = std::make_pair(pid, tid);
    return _connections[conn_id];
}

std::pair<int, int> System::GetThreadFromConnection(intptr_t conn_id)
{
    auto it = _connections.find(conn_id);
    if (it == _connections.end())
        return std::make_pair(-1, -1);
    return it->second;
}

size_t System::UnregisterConnection(intptr_t conn_id)
{
    _connections.erase(conn_id);
    return _connections.size();
}

int System::RegisterPort(std::shared_ptr<Port>&& port)
{
    int id = _ports.Add(port);
    port->_info.port = id;
    // Haiku doesn't seem to do anything
    // about ports with duplicate names,
    // so neither should we.
    _portNames.emplace(port->GetName(), id);
    return id;
}

std::weak_ptr<Port> System::GetPort(int port_id)
{
    if (_ports.IsValidId(port_id))
    {
        return _ports.Get(port_id);
    }
    return std::weak_ptr<Port>();
}

int System::FindPort(const std::string& portName)
{
    auto it = _portNames.find(portName);
    if (it != _portNames.end())
    {
        return it->second;
    }
    return -1;
}

size_t System::UnregisterPort(int port_id)
{
    std::shared_ptr<Port> port = _ports.Get(port_id);
    if (port)
    {
        _portNames.erase(port->GetName());
        _ports.Remove(port_id);
    }
    return _ports.Size();
}

int System::CreateSemaphore(int pid, int count, const char* name)
{
    std::shared_ptr<Semaphore> semaphore = std::make_shared<Semaphore>(pid, count, name);
    int id = _semaphores.Add(semaphore);
    semaphore->_info.sem = id;
    semaphore->_registered = true;
    return id;
}

std::weak_ptr<Semaphore> System::GetSemaphore(int id)
{
    if (_semaphores.IsValidId(id))
    {
        return _semaphores.Get(id);
    }
    return std::weak_ptr<Semaphore>();
}

size_t System::UnregisterSemaphore(int id)
{
    if (_semaphores.IsValidId(id))
    {
        auto sem = _semaphores.Get(id);
        sem->_registered = false;
        sem->_countCondVar.notify_all();
        _semaphores.Remove(id);
    }
    return _semaphores.Size();
}

int System::RegisterFSInfo(std::shared_ptr<haiku_fs_info>&& info)
{
    int id = _fsInfos.Add(info);
    // Don't add the id here.
    // info->dev = id;
    return id;
}

// Finds a fs_info by hyclone internal id.
std::weak_ptr<haiku_fs_info> System::GetFSInfo(int id)
{
    if (_fsInfos.IsValidId(id))
    {
        return _fsInfos.Get(id);
    }
    return std::weak_ptr<haiku_fs_info>();
}

// Finds a fs_info by system dev_t id.
std::weak_ptr<haiku_fs_info> System::FindFSInfoByDevId(int devId)
{
    for (auto& info : _fsInfos)
    {
        if (info->dev == devId)
        {
            return info;
        }
    }
    return std::weak_ptr<haiku_fs_info>();
}

int System::NextFSInfoId(int id) const
{
    return _fsInfos.NextId(id);
}

int System::RegisterArea(const haiku_area_info& info)
{
    int nextId = _nextAreaId;
    _nextAreaId = (_nextAreaId == INT_MAX) ? 1 : _nextAreaId + 1;

    while (_areas.find(nextId) != _areas.end())
    {
        nextId = _nextAreaId;
        _nextAreaId = (_nextAreaId == INT_MAX) ? 1 : _nextAreaId + 1;
    }

    std::cerr << "Registering area " << nextId << " with size " << info.size << " for pid " << info.team << std::endl;

    _areas[nextId] = info;
    _areas[nextId].area = nextId;
    return nextId;
}

bool System::IsValidAreaId(int id) const
{
    return _areas.find(id) != _areas.end();
}

const haiku_area_info& System::GetArea(int id) const
{
    return _areas.at(id);
}

haiku_area_info& System::GetArea(int id)
{
    std::cerr << _areas.contains(id) << std::endl;
    return _areas.at(id);
}

size_t System::UnregisterArea(int id)
{
    _areas.erase(id);
    return _areas.size();
}

int System::RegisterEntryRef(const EntryRef& ref, const std::string& path)
{
    std::string tmpPath = path;
    tmpPath.shrink_to_fit();
    return RegisterEntryRef(ref, std::move(tmpPath));
}

int System::RegisterEntryRef(const EntryRef& ref, std::string&& path)
{
    // TODO: Limit the total number of entryRefs stored.
    _entryRefs[ref] = path;
    return 0;
}

int System::GetEntryRef(const EntryRef& ref, std::string& path) const
{
    auto it = _entryRefs.find(ref);
    if (it != _entryRefs.end())
    {
        path.resize(it->second.size());
        memcpy(path.data(), it->second.c_str(), it->second.size() + 1);
        return 0;
    }
    return -1;
}

void System::Shutdown()
{
    _isShuttingDown = true;
    for (const auto& process : _processes)
    {
        server_kill_process(process.first);
    }
    for (const auto& connection : _connections)
    {
        server_close_connection(connection.first);
    }
}

intptr_t server_hserver_call_connect(hserver_context& context, int pid, int tid)
{
    auto& system = System::GetInstance();
    auto lock = system.Lock();
    system.RegisterConnection(context.conn_id, pid, tid);

    std::cerr << "Connection started: " << context.conn_id << " " << pid << " " << tid << std::endl;

    // Might already be registered by fork().
    auto process = system.GetProcess(pid).lock();
    if (!process)
    {
        process = system.RegisterProcess(pid).lock();
        std::cerr << "Registered process: " << context.conn_id << " " << pid << std::endl;
    }

    if (process)
    {
        auto thread = system.RegisterThread(pid, tid).lock();
        if (thread)
        {
            process->RegisterThread(thread);
            return B_OK;
        }
    }

    return HAIKU_POSIX_ENOMEM;
}

intptr_t server_hserver_call_disconnect(hserver_context& context)
{
    auto& system = System::GetInstance();
    auto lock = system.Lock();
    auto procLock = context.process->Lock();
    // No more threads => Process dead.
    if (!context.process->UnregisterThread(context.tid))
    {
        system.UnregisterProcess(context.pid);
        for (const auto& s: context.process->GetOwningSemaphores())
        {
            system.UnregisterSemaphore(s);
        }

        for (const auto& s: context.process->GetOwningPorts())
        {
            system.UnregisterPort(s);
        }

        area_id area = -1;
        while ((area = context.process->NextAreaId(area)) != -1)
        {
            system.UnregisterArea(area);
        }
    }
    system.UnregisterThread(context.tid);
    system.UnregisterConnection(context.conn_id);

    std::cerr << "Unregistered: " << context.conn_id << " " << context.pid << " " << context.tid << std::endl;

    return B_OK;
}

intptr_t server_hserver_call_fork(hserver_context& context, int newPid)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Process> child;

    {
        auto lock = system.Lock();

        // The newPid should already exist.
        std::cerr << "Registering fork: parent: " << context.pid << " child: " << newPid << std::endl;

        child = system.GetProcess(newPid).lock();

        if (!child)
        {
            system.RegisterProcess(newPid);
            std::cerr << "Registered process: " << context.conn_id << " " << newPid << " from fork parent " << context.pid << std::endl;
            child = system.GetProcess(newPid).lock();
        }

        auto parentLock = context.process->Lock();
        auto childLock = child->Lock();

        // Copy information, especially area info, as soon as possible.
        // Else, calls such as `area_for` may fail before the copy is complete.
        context.process->Fork(*child);
        std::cerr << context.pid << " unlocked fork for " << newPid << "." << std::endl;
    }

    return B_OK;
}

intptr_t server_hserver_call_wait_for_fork_unlock(hserver_context& context)
{
    while (true)
    {
        {
            auto procLock = context.process->Lock();

            if (context.process->IsForkUnlocked())
            {
                return B_OK;
            }
        }

        std::cerr << context.process->GetPid() <<  ": Fork not unlocked." << std::endl;
        // Sleep for a short time as fork happens quite quickly.
        server_worker_sleep(50);
    }

    // Should not reach here.
    std::cerr << "server_hserver_call_wait_for_fork_unlock: Impossible code path reached." << std::endl;
    return B_BAD_DATA;
}