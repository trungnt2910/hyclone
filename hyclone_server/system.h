#ifndef __HYCLONE_SYSTEM_H__
#define __HYCLONE_SYSTEM_H__

#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

class Process;

class System
{
private:
    bool _isShuttingDown = false;
    std::unordered_map<int, std::shared_ptr<Process>> _processes;
    std::unordered_map<intptr_t, std::pair<int, int>> _connections;
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

    void Shutdown();
    bool IsShuttingDown() const { return _isShuttingDown; }

    std::unique_lock<std::mutex> Lock() { return std::unique_lock<std::mutex>(_lock); }

    static System& GetInstance();
};

#endif // __HYCLONE_SYSTEM_H__