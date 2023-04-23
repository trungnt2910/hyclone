#include <cstddef>
#include <cstring>
#include <iostream>

#include "area.h"
#include "entry_ref.h"
#include "haiku_area.h"
#include "haiku_debugger.h"
#include "haiku_errors.h"
#include "haiku_fs_info.h"
#include "haiku_scheduler.h"
#include "hsemaphore.h"
#include "port.h"
#include "process.h"
#include "server_native.h"
#include "server_servercalls.h"
#include "server_systemnotification.h"
#include "server_workers.h"
#include "system.h"
#include "thread.h"

System& System::GetInstance()
{
    static System instance;
    return instance;
}

status_t System::Init()
{
    status_t status = B_OK;

    if ((status = _teamNotificationService.Register()) != B_OK)
    {
        return status;
    }

    if ((status = _threadNotificationService.Register()) != B_OK)
    {
        return status;
    }

    if ((status = _systemNotificationService.Init()) != B_OK)
    {
        return status;
    }

    return status;
}

std::weak_ptr<Process> System::RegisterProcess(int pid, int uid, int gid, int euid, int egid)
{
    auto ptr = std::make_shared<Process>(pid, uid, gid, euid, egid);
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
    ptr->_registered = true;
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
    auto it = _threads.find(tid);
    if (it != _threads.end())
    {
        it->second->_registered = false;
        it->second->_blockCondition.notify_all();
        it->second->_sendDataCondition.notify_all();
        it->second->_receiveDataCondition.notify_all();
        it->second->_suspended = false;
        it->second->_suspended.notify_all();
        _threads.erase(it);
    }
    return _threads.size();
}

const Connection& System::RegisterConnection(intptr_t conn_id, const Connection& conn)
{
    auto result = _connections.emplace(conn_id, conn);
    return result.first->second;
}

Connection System::GetThreadFromConnection(intptr_t conn_id)
{
    auto it = _connections.find(conn_id);
    if (it == _connections.end())
        return Connection(-1, -1);
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
    port->_registered = true;
    // Haiku doesn't seem to do anything
    // about ports with duplicate names,
    // so neither should we.
    _portNames.emplace(port->GetName(), id);
    return id;
}

std::weak_ptr<Port> System::GetPort(int portId)
{
    if (_ports.IsValidId(portId))
    {
        return _ports.Get(portId);
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

size_t System::UnregisterPort(int portId)
{
    std::shared_ptr<Port> port = _ports.Get(portId);
    if (port)
    {
        _portNames.erase(port->GetName());
        _ports.Remove(portId);
        port->_registered = false;
        port->_readCondVar.notify_all();
        port->_writeCondVar.notify_all();
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

std::weak_ptr<Area> System::RegisterArea(const haiku_area_info& info)
{
    int nextId = _nextAreaId;
    _nextAreaId = (_nextAreaId == INT_MAX) ? 1 : _nextAreaId + 1;

    while (_areas.find(nextId) != _areas.end())
    {
        nextId = _nextAreaId;
        _nextAreaId = (_nextAreaId == INT_MAX) ? 1 : _nextAreaId + 1;
    }

    auto ptr = std::make_shared<Area>(info);
    ptr->_info.area = nextId;
    _areas[nextId] = ptr;
    return ptr;
}

std::weak_ptr<Area> System::RegisterArea(const std::shared_ptr<Area>& ptr)
{
    int nextId = _nextAreaId;
    _nextAreaId = (_nextAreaId == INT_MAX) ? 1 : _nextAreaId + 1;

    while (_areas.find(nextId) != _areas.end())
    {
        nextId = _nextAreaId;
        _nextAreaId = (_nextAreaId == INT_MAX) ? 1 : _nextAreaId + 1;
    }

    if (ptr->IsShared())
    {
        auto lock = _memoryService.Lock();
        if (!_memoryService.AcquireSharedFile(ptr->_entryRef))
        {
            return std::weak_ptr<Area>();
        }
    }

    ptr->_info.area = nextId;
    _areas[nextId] = ptr;
    return ptr;
}

bool System::IsValidAreaId(int id) const
{
    return _areas.find(id) != _areas.end();
}

std::weak_ptr<Area> System::GetArea(int id)
{
    auto it = _areas.find(id);
    if (it == _areas.end())
        return std::weak_ptr<Area>();
    return it->second;
}

size_t System::UnregisterArea(int id)
{
    if (_areas.contains(id))
    {
        const auto& area = _areas.at(id);
        if (area->IsShared())
        {
            auto lock = _memoryService.Lock();
            _memoryService.ReleaseSharedFile(area->_entryRef);
        }
        _areas.erase(id);
    }
    return _areas.size();
}

void System::Shutdown()
{
    _isShuttingDown = true;
    for (const auto& process : _processes)
    {
        server_kill_process(process.first);
    }
    for (const auto& [id, area] : _areas)
    {
        if (area->IsShared())
        {
            _memoryService.ReleaseSharedFile(area->_entryRef);
        }
    }
    for (const auto& connection : _connections)
    {
        server_close_connection(connection.first);
    }
}

intptr_t server_hserver_call_connect(hserver_context& context, int pid, int tid,
    intptr_t uid, intptr_t gid, intptr_t euid, intptr_t egid)
{
    std::shared_ptr<Process> process;

    auto& system = System::GetInstance();

    std::cerr << "Connection started: " << context.conn_id << " " << pid << " " << tid << std::endl;

    bool processNewlyCreated = false;

    {
        auto lock = system.Lock();
        system.RegisterConnection(context.conn_id, Connection(pid, tid));

        // Might already be registered by fork() or left behind after an exec().
        process = system.GetProcess(pid).lock();

        if (!process)
        {
            {
                auto& mapService = system.GetUserMapService();
                auto mapLock = mapService.Lock();
                uid = mapService.GetUid(uid);
                gid = mapService.GetGid(gid);
                euid = mapService.GetUid(euid);
                egid = mapService.GetGid(egid);
            }
            process = system.RegisterProcess(pid, uid, gid, euid, egid).lock();
            if (process)
            {
                processNewlyCreated = true;
            }
            std::cerr << "Registered process: " << context.conn_id << " " << pid << std::endl;
        }
        else
        {
            std::cerr << "Process " << pid << " already registered" << std::endl;
        }
    }

    if (process)
    {
        if (processNewlyCreated)
        {
            system.GetTeamNotificationService().Notify(TEAM_ADDED, process);
        }
        else
        {
            process->WaitForExec();
        }
        std::shared_ptr<Thread> thread;
        {
            auto lock = system.Lock();
            thread = system.RegisterThread(pid, tid).lock();
        }
        if (thread)
        {
            system.GetTeamNotificationService().Notify(THREAD_ADDED, process);
            auto lock = process->Lock();
            process->RegisterThread(thread);
            return B_OK;
        }
    }

    return HAIKU_POSIX_ENOMEM;
}

intptr_t server_hserver_call_disconnect(hserver_context& context)
{
    std::shared_ptr<Port> debuggerPort;
    std::shared_ptr<Semaphore> debuggerWriteLock;
    std::vector<Port::Message> debuggerMessages;
    std::vector<int> teamNotificationEvents;
    std::vector<int> threadNotificationEvents;
    bool processEnded = false;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        const auto& connection = system.GetThreadFromConnection(context.conn_id);

        bool execUnlockNeeded = false;

        if (context.process && connection.isPrimary)
        {
            auto procLock = context.process->Lock();
            // No more threads => Process dead.
            if (!context.process->UnregisterThread(context.tid))
            {
                if (!context.process->IsExecutingExec())
                {
                    system.UnregisterProcess(context.pid);
                }
                else
                {
                    std::cerr << "Keeping a reference to process " << context.pid << " because it is executing exec." << std::endl;
                    execUnlockNeeded = true;
                }

                processEnded = true;

                context.process->PrepareForDeletion();

                for (const auto& s: context.process->GetOwningSemaphores())
                {
                    system.UnregisterSemaphore(s);
                }
                context.process->ClearOwningSemaphores();

                for (const auto& s: context.process->GetOwningPorts())
                {
                    system.UnregisterPort(s);
                }
                context.process->ClearOwningPorts();

                area_id area = -1;
                while ((area = context.process->NextAreaId(area)) != -1)
                {
                    system.UnregisterArea(area);
                }
                context.process->ClearAreas();

                context.process->ClearImages();

                teamNotificationEvents.push_back(context.process->IsExecutingExec() ?
                    TEAM_EXEC : TEAM_REMOVED);

                if (context.process->GetDebuggerPort() != -1 && !context.process->IsExecutingExec())
                {
                    if (context.process->GetInfo().debugger_nub_port != -1)
                    {
                        system.UnregisterPort(context.process->GetInfo().debugger_nub_port);
                    }

                    Port::Message message;
                    message.code = B_DEBUGGER_MESSAGE_TEAM_DELETED;
                    message.data.resize(sizeof(debug_debugger_message_data));
                    memset(&message.info, 0, sizeof(message.info));
                    message.info.sender_team = context.pid;
                    message.info.size = sizeof(debug_debugger_message_data);

                    auto& debuggerMessage = *(debug_debugger_message_data*)message.data.data();
                    debuggerMessage.team_deleted.origin.team = context.pid;
                    debuggerMessage.team_deleted.origin.thread = -1;
                    debuggerMessage.team_deleted.origin.nub_port = -1;

                    debuggerMessages.emplace_back(std::move(message));
                }
            }

            system.UnregisterThread(context.tid);

            threadNotificationEvents.push_back(THREAD_REMOVED);

            if (context.tid != context.pid || !context.process->IsExecutingExec())
            {
                if (context.process->GetDebuggerPort() != -1)
                {
                    Port::Message message;
                    message.code = B_DEBUGGER_MESSAGE_THREAD_DELETED;
                    message.data.resize(sizeof(debug_debugger_message_data));
                    memset(&message.info, 0, sizeof(message.info));
                    message.info.sender_team = context.pid;
                    message.info.size = sizeof(debug_debugger_message_data);

                    auto& debuggerMessage = *(debug_debugger_message_data*)message.data.data();
                    debuggerMessage.thread_deleted.origin.team = context.pid;
                    debuggerMessage.thread_deleted.origin.thread = context.tid;
                    debuggerMessage.thread_deleted.origin.nub_port = -1;
                }
            }

            if (debuggerMessages.size())
            {
                debuggerPort = system.GetPort(context.process->GetDebuggerPort()).lock();
                debuggerWriteLock = system.GetSemaphore(context.process->GetDebuggerWriteLock()).lock();
            }
        }

        if (execUnlockNeeded)
        {
            std::cerr << "Unlocking exec for process " << context.pid << std::endl;
            context.process->Exec(false);
        }

        system.UnregisterConnection(context.conn_id);

        std::cerr << "Unregistered: " << context.conn_id << " " << context.pid << " " << context.tid << std::endl;
    }

    if (debuggerMessages.size() && debuggerPort && debuggerWriteLock)
    {
        debuggerWriteLock->Acquire(context.tid, 1);

        for (auto& message: debuggerMessages)
        {
            debuggerPort->Write(std::move(message), B_INFINITE_TIMEOUT);
        }

        debuggerWriteLock->Release(1);
    }

    if (processEnded)
    {
        auto& msgService = System::GetInstance().GetMessagingService();
        auto msgLock = msgService.Lock();

        // Will silently fail if the process is not the registered
        // message server.
        msgService.UnregisterService(context.process);
    }

    for (const auto& event: teamNotificationEvents)
    {
        System::GetInstance().GetTeamNotificationService().Notify(event, context.process);
    }

    for (const auto& event: threadNotificationEvents)
    {
        System::GetInstance().GetThreadNotificationService().Notify(event, context.thread);
    }

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
            system.RegisterProcess(newPid,
                context.process->GetUid(),
                context.process->GetGid(),
                context.process->GetEuid(),
                context.process->GetEgid());
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

    if (child)
    {
        system.GetTeamNotificationService().Notify(TEAM_ADDED, child);
    }

    return B_OK;
}

intptr_t server_hserver_call_wait_for_fork_unlock(hserver_context& context)
{
    context.process->WaitForForkUnlock();
    return B_OK;
}

intptr_t server_hserver_call_get_safemode_option(hserver_context& context, const char* userParameter, size_t parameterSize,
    char* userBuffer, size_t* userBufferSize)
{
    // TODO: Allow passing safe mode options as HyClone command line arguments.
    std::string parameter(parameterSize, '\0');

    {
        auto lock = context.process->Lock();
        if (context.process->ReadMemory((void*)userParameter, parameter.data(), parameterSize) != parameterSize)
        {
            return B_BAD_ADDRESS;
        }
    }

    const char* env = getenv(parameter.c_str());

    if (!env)
    {
        return B_ENTRY_NOT_FOUND;
    }

    size_t envSize = strlen(env) + 1;
    size_t bufferSize;

    {
        auto lock = context.process->Lock();
        if (context.process->ReadMemory(userBuffer, &bufferSize, sizeof(bufferSize)) != sizeof(bufferSize))
        {
            return B_BAD_ADDRESS;
        }

        size_t writeSize = std::min(bufferSize, envSize);
        if (context.process->WriteMemory(userBuffer, env, writeSize) != writeSize)
        {
            return B_BAD_ADDRESS;
        }
        if (context.process->WriteMemory(userBufferSize, &envSize, sizeof(envSize)) != sizeof(envSize))
        {
            return B_BAD_ADDRESS;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_set_scheduler_mode(hserver_context& context, int mode)
{
    auto& system = System::GetInstance();

    if (mode != SCHEDULER_MODE_LOW_LATENCY && mode != SCHEDULER_MODE_POWER_SAVING)
    {
        return B_BAD_VALUE;
    }

    {
        auto lock = system.Lock();
        system.SetSchedulerMode(mode);
    }

    return B_OK;
}

intptr_t server_hserver_call_get_scheduler_mode(hserver_context& context)
{
    auto& system = System::GetInstance();
    auto lock = system.Lock();
    return system.GetSchedulerMode();
}