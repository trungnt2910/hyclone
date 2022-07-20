#include <cstddef>
#include <iostream>

#include "haiku_errors.h"
#include "port.h"
#include "process.h"
#include "server_native.h"
#include "server_servercalls.h"
#include "system.h"

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

size_t System::UnregisterProcess(int pid)
{
    _processes.erase(pid);
    return _processes.size();
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
    _portNames.emplace(port->Name(), id);
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
        _portNames.erase(port->Name());
        _ports.Remove(port_id);
    }
    return _ports.Size();
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

    std::cerr << "Registered: " << context.conn_id << " " << pid << " " << tid << std::endl;

    // Might already be registered by fork().
    auto process = system.GetProcess(pid).lock();
    if (!process)
    {
        process = system.RegisterProcess(pid).lock();
    }

    if (process)
    {
        process->RegisterThread(tid);
        return B_OK;
    }
    else
    {
        return HAIKU_POSIX_ENOMEM;
    }
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
    }
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
            child = system.GetProcess(newPid).lock();
        }
    }

    auto parentLock = context.process->Lock();
    auto childLock = child->Lock();

    context.process->Fork(*child);

    return B_OK;
}
