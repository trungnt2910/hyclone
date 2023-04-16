#ifndef __HYCLONE_PORT_H__
#define __HYCLONE_PORT_H__

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>

#include "haiku_port.h"

class System;

class Port
{
    // Allows the system to set the port ID.
    friend class System;
public:
    struct Message
    {
        std::vector<char> data;
        haiku_port_message_info info;
        int code;
    };
private:
    haiku_port_info _info;
    std::queue<Message> _messages;
    std::mutex _messagesLock;
    std::condition_variable _writeCondVar;
    std::condition_variable _readCondVar;
    std::mutex _lock;
    std::atomic<bool> _registered = false;
    bool _closed = false;
public:
    Port(int pid, int capacity, const char* name);
    ~Port() = default;

    status_t Write(Message&& message, bigtime_t timeout);
    status_t Read(Message& message, bigtime_t timeout);
    status_t GetMessageInfo(haiku_port_message_info& info, bigtime_t timeout);
    status_t Close();

    std::string GetName() const { return _info.name; }
    const haiku_port_info& GetInfo() const { return _info; }
    size_t GetBufferSize() const { return _messages.front().data.size(); }
    int GetOwner() const { return _info.team; }
    int GetId() const { return _info.port; }

    bool IsClosed() const { return _closed; }

    std::unique_lock<std::mutex> Lock() { return std::unique_lock<std::mutex>(_lock); }

    void SetOwner(int team) { _info.team = team; }
};

#define HAIKU_PORT_MAX_QUEUE_LENGTH 4096

#endif // __HYCLONE_PORT_H__
