#ifndef __HYCLONE_PORT_H__
#define __HYCLONE_PORT_H__

#include <atomic>
#include <mutex>
#include <queue>
#include <string>

#include "haiku_port.h"

class System;

class Port
{
    // Allows the system to set the port ID.
    friend class System;
private:
    haiku_port_info _info;
    std::queue<std::vector<char>> _messages;
    std::mutex _lock;
public:
    Port(int pid, int capacity, const char* name);
    ~Port() = default;

    std::unique_lock<std::mutex> Lock() { return std::unique_lock(_lock); }

    void Write(std::vector<char>&& message);
    std::vector<char> Read();

    std::string GetName() const { return _info.name; }
    const haiku_port_info& GetInfo() const { return _info; }

    void SetOwner(int team) { _info.team = team; }
};

#define HAIKU_PORT_MAX_QUEUE_LENGTH 4096

#endif // __HYCLONE_PORT_H__
