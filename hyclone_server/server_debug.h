#ifndef __SERVER_DEBUG_H__
#define __SERVER_DEBUG_H__

#include <memory>
#include <mutex>

#include "port.h"

class DebugService
{
private:
    std::weak_ptr<Port> _defaultDebuggerPort;
    std::mutex _lock;
public:
    DebugService() = default;
    ~DebugService() = default;

    void SetDefaultDebuggerPort(const std::shared_ptr<Port>& port) { _defaultDebuggerPort = port; }

    std::unique_lock<std::mutex> Lock() { return std::unique_lock(_lock); }
};

#endif