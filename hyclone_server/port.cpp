#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include "BeDefs.h"
#include "haiku_errors.h"
#include "port.h"
#include "process.h"
#include "server_native.h"
#include "server_servercalls.h"
#include "server_workers.h"
#include "system.h"

const int kSleepTimeMicroseconds = 100 * 1000; // 100ms, in microseconds.

Port::Port(int pid, int capacity, const char* name)
{
    _info.capacity = capacity;
    _info.team = pid;
    strncpy(_info.name, name, sizeof(_info.name) - 1);
    _info.name[sizeof(_info.name) - 1] = '\0';
    _info.queue_count = 0;
    _info.total_count = 0;
    _info.port = 0;
}

status_t Port::Write(Message&& message, bigtime_t timeout)
{
    message.info.size == message.data.size();

    std::unique_lock<std::mutex> lock(_messagesLock);

    if (_info.queue_count == _info.capacity)
    {
        if (timeout == 0)
        {
            return B_WOULD_BLOCK;
        }

        server_worker_run_wait([&]()
        {
            const auto writableOrDead = [&]()
            {
                return !_registered || _info.queue_count < _info.capacity;
            };

            if (timeout != B_INFINITE_TIMEOUT)
            {
                _writeCondVar.wait_for(lock, std::chrono::microseconds(timeout), writableOrDead);
                return;
            }
            else
            {
                _writeCondVar.wait(lock, writableOrDead);
            }
        });
    }

    if (!_registered)
    {
        return B_BAD_PORT_ID;
    }

    if (_info.queue_count == _info.capacity)
    {
        return B_TIMED_OUT;
    }

    _messages.emplace(std::move(message));
    ++_info.queue_count;

    _readCondVar.notify_one();

    return B_OK;
}

status_t Port::Read(Message& message, bigtime_t timeout)
{
    std::unique_lock<std::mutex> lock(_messagesLock);

    if (_info.queue_count == 0)
    {
        if (timeout == 0)
        {
            return B_WOULD_BLOCK;
        }

        server_worker_run_wait([&]()
        {
            const auto readableOrDead = [&]()
            {
                return !_registered || _info.queue_count > 0;
            };

            if (timeout != B_INFINITE_TIMEOUT)
            {
                _readCondVar.wait_for(lock, std::chrono::microseconds(timeout), readableOrDead);
                return;
            }
            else
            {
                _readCondVar.wait(lock, readableOrDead);
            }
        });
    }

    if (!_registered)
    {
        return B_BAD_PORT_ID;
    }

    if (_info.queue_count == 0)
    {
        return B_TIMED_OUT;
    }

    --_info.queue_count;
    ++_info.total_count;

    message = std::move(_messages.front());
    _messages.pop();

    _writeCondVar.notify_one();

    return B_OK;
}

status_t Port::GetMessageInfo(haiku_port_message_info& info, bigtime_t timeout)
{
    std::unique_lock<std::mutex> lock(_messagesLock);

    if (_info.queue_count == 0)
    {
        if (timeout == 0)
        {
            return B_WOULD_BLOCK;
        }

        server_worker_run_wait([&]()
        {
            const auto readableOrDead = [&]()
            {
                return !_registered || _info.queue_count > 0;
            };

            if (timeout != B_INFINITE_TIMEOUT)
            {
                _readCondVar.wait_for(lock, std::chrono::microseconds(timeout), readableOrDead);
                return;
            }
            else
            {
                _readCondVar.wait(lock, readableOrDead);
            }
        });
    }

    if (!_registered)
    {
        return B_BAD_PORT_ID;
    }

    if (_info.queue_count == 0)
    {
        return B_TIMED_OUT;
    }

    info = _messages.front().info;

    // We haven't read anything, let someone else read it.
    _readCondVar.notify_one();

    return B_OK;
}

intptr_t server_hserver_call_create_port(hserver_context& context, int32 queue_length, const char *name, size_t portNameLength)
{
    if (queue_length < 1 || queue_length > HAIKU_PORT_MAX_QUEUE_LENGTH)
    {
        return B_BAD_VALUE;
    }

    auto buffer = std::string(portNameLength, '\0');
    if (server_read_process_memory(context.pid, (void*)name, buffer.data(), portNameLength) != portNameLength)
    {
        return B_BAD_ADDRESS;
    }
    auto newPort = std::make_shared<Port>(context.pid, queue_length, buffer.c_str());

    int id;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        id = system.RegisterPort(std::move(newPort));
    }

    {
        auto lock = context.process->Lock();
        context.process->AddOwningPort(id);
    }

    return id;
}

intptr_t server_hserver_call_delete_port(hserver_context& context, int portId)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Port> port;
    std::shared_ptr<Process> owner;

    {
        auto lock = system.Lock();

        port = system.GetPort(portId).lock();
        if (!port)
        {
            return B_BAD_PORT_ID;
        }

        owner = system.GetProcess(port->GetOwner()).lock();
        if (!owner)
        {
            std::cerr << "Port #" << port->GetId() << " (" << port->GetName() << ") owned by dead process " << port->GetInfo().team << std::endl;
            return B_BAD_PORT_ID;
        }
    }

    {
        auto lock = owner->Lock();
        owner->RemoveOwningPort(portId);
    }

    {
        auto lock = system.Lock();
        system.UnregisterPort(portId);
    }
    return B_OK;
}

intptr_t server_hserver_call_find_port(hserver_context& context, const char *port_name, size_t portNameLength)
{
    auto buffer = std::string(portNameLength, '\0');
    if (server_read_process_memory(context.pid, (void*)port_name, buffer.data(), portNameLength) != portNameLength)
    {
        return B_BAD_ADDRESS;
    }

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        int result = system.FindPort(buffer);

        if (result < 0)
        {
            return B_NAME_NOT_FOUND;
        }

        return result;
    }
}

intptr_t server_hserver_call_get_port_info(hserver_context& context, port_id id, void *info)
{
    std::shared_ptr<Port> port;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        port = system.GetPort(id).lock();
    }

    if (!port)
    {
        return B_BAD_PORT_ID;
    }

    {
        auto lock = context.process->Lock();
        if (server_write_process_memory(context.pid, info, &port->GetInfo(), sizeof(haiku_port_info))
            != sizeof(haiku_port_info))
        {
            return B_BAD_ADDRESS;
        }

        return B_OK;
    }
}

intptr_t server_hserver_call_port_count(hserver_context& context, port_id id)
{
    std::shared_ptr<Port> port;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        port = system.GetPort(id).lock();
    }

    if (!port)
    {
        return B_BAD_PORT_ID;
    }

    return port->GetInfo().queue_count;
}

intptr_t server_hserver_call_port_buffer_size_etc(hserver_context& context, port_id id,
    uint32 flags, unsigned long long timeout)
{
    std::shared_ptr<Port> port;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        port = system.GetPort(id).lock();
    }

    if (!port)
    {
        return B_BAD_PORT_ID;
    }

    haiku_port_message_info messageInfo;
    bool useTimeout = flags & B_TIMEOUT;

    status_t status = port->GetMessageInfo(messageInfo, useTimeout ? timeout : B_INFINITE_TIMEOUT);

    if (status != B_OK)
    {
        return status;
    }

    return messageInfo.size;
}

intptr_t server_hserver_call_set_port_owner(hserver_context& context, port_id id, team_id team)
{
    std::shared_ptr<Port> port;
    std::shared_ptr<Process> oldOwner;
    std::shared_ptr<Process> newOwner;
    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        port = system.GetPort(id).lock();

        if (!port)
        {
            return B_BAD_PORT_ID;
        }

        oldOwner = system.GetProcess(port->GetInfo().team).lock();

        // Owner dead so port probably dead?
        if (!oldOwner)
        {
            std::cerr << "Port " << port->GetName() << " owned by dead process " << port->GetInfo().team << std::endl;
            return B_BAD_PORT_ID;
        }

        newOwner = system.GetProcess(team).lock();

        if (!newOwner)
        {
            return B_BAD_TEAM_ID;
        }
    }

    if (oldOwner != newOwner)
    {
        auto oldOwnerLock = oldOwner->Lock();
        auto newOwnerLock = newOwner->Lock();
        auto portLock = port->Lock();
        oldOwner->RemoveOwningPort(id);
        newOwner->AddOwningPort(id);
        port->SetOwner(team);
    }

    return B_OK;
}

intptr_t server_hserver_call_write_port_etc(hserver_context& context, port_id id, int32 messageCode, const void *msgBuffer,
    size_t bufferSize, uint32 flags, unsigned long long timeout)
{
    std::shared_ptr<Port> port;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        port = system.GetPort(id).lock();
    }

    if (!port)
    {
        return B_BAD_PORT_ID;
    }

    Port::Message message;
    message.code = messageCode;
    message.data.resize(bufferSize);
    memset(&message.info, 0, sizeof(message.info));
    message.info.sender_team = context.pid;
    message.info.size = bufferSize;

    if (server_read_process_memory(context.pid, (void*)msgBuffer, message.data.data(), bufferSize)
        != bufferSize)
    {
        return B_BAD_ADDRESS;
    }

    bool useTimeout = flags & B_TIMEOUT;

    return port->Write(std::move(message), useTimeout ? timeout : B_INFINITE_TIMEOUT);
}

intptr_t server_hserver_call_read_port_etc(hserver_context& context,
    port_id id, int32* userMessageCode, void *msgBuffer,
    size_t bufferSize, uint32 flags, unsigned long long timeout)
{
    std::shared_ptr<Port> port;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        port = system.GetPort(id).lock();
    }

    if (!port)
    {
        return B_BAD_PORT_ID;
    }

    Port::Message message;

    bool useTimeout = flags & B_TIMEOUT;

    status_t status = port->Read(message, useTimeout ? timeout : B_INFINITE_TIMEOUT);

    if (status != B_OK)
    {
        return status;
    }

    if (context.process->WriteMemory(userMessageCode, &message.code, sizeof(message.code)) != sizeof(message.code))
    {
        return B_BAD_ADDRESS;
    }

    size_t writeSize = std::min(message.data.size(), bufferSize);
    if (context.process->WriteMemory(msgBuffer, message.data.data(), writeSize) != writeSize)
    {
        return B_BAD_ADDRESS;
    }

    return B_OK;
}

intptr_t server_hserver_call_get_port_message_info_etc(hserver_context& context, int id,
    void* userPortMessageInfo, size_t infoSize, unsigned int flags, unsigned long long timeout)
{
    if (infoSize != sizeof(haiku_port_message_info))
    {
        return B_BAD_VALUE;
    }

    std::shared_ptr<Port> port;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        port = system.GetPort(id).lock();
    }

    if (!port)
    {
        return B_BAD_PORT_ID;
    }

    haiku_port_message_info messageInfo;
    bool useTimeout = flags & B_TIMEOUT;

    status_t status = port->GetMessageInfo(messageInfo, useTimeout ? timeout : B_INFINITE_TIMEOUT);

    if (status != B_OK)
    {
        return status;
    }

    if (context.process->WriteMemory(userPortMessageInfo, &messageInfo, infoSize) != infoSize)
    {
        return B_BAD_ADDRESS;
    }

    return B_OK;
}