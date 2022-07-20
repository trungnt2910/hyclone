#ifndef __HYCLONE_SYSTEM_H__
#define __HYCLONE_SYSTEM_H__

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "id_map.h"

class Process;
class Port;

class System
{
private:
    bool _isShuttingDown = false;
    std::unordered_map<int, std::shared_ptr<Process>> _processes;
    std::unordered_map<intptr_t, std::pair<int, int>> _connections;
    IdMap<std::shared_ptr<Port>, int> _ports;
    std::unordered_map<std::string, int> _portNames;
    std::mutex _lock;
    System() = default;
    ~System() = default;
public:
    System(const System&) = delete;
    System(System&&) = delete;
    std::weak_ptr<Process> RegisterProcess(int pid);
    std::weak_ptr<Process> GetProcess(int pid);
    size_t UnregisterProcess(int pid);

    std::pair<int, int> RegisterConnection(intptr_t conn_id, int pid, int tid);
    std::pair<int, int> GetThreadFromConnection(intptr_t conn_id);
    size_t UnregisterConnection(intptr_t conn_id);

    int RegisterPort(std::shared_ptr<Port>&& port);
    std::weak_ptr<Port> GetPort(int port_id);
    int FindPort(const std::string& portName);
    size_t UnregisterPort(int port_id);

    void Shutdown();
    bool IsShuttingDown() const { return _isShuttingDown; }

    std::unique_lock<std::mutex> Lock() { return std::unique_lock<std::mutex>(_lock); }

    static System& GetInstance();
};

#endif // __HYCLONE_SYSTEM_H__