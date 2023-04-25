#include "io_context.h"
#include "server_nodemonitor.h"

IoContext::IoContext(const IoContext& other)
{
    _maxMonitors = other._maxMonitors;

    // Monitors are not copied.
}

IoContext& IoContext::operator=(const IoContext& other)
{
    if (this == &other)
        return *this;

    _maxMonitors = other._maxMonitors;

    return *this;
}

unsigned int IoContext::NumMonitors() const
{
    return _monitors.size();
}

size_t IoContext::AddMonitor(const std::shared_ptr<monitor_listener>& listener)
{
    _monitors.push_back(listener);
    listener->context_link = --_monitors.end();
    return _monitors.size();
}

size_t IoContext::RemoveMonitor(const std::shared_ptr<monitor_listener>& listener)
{
    _monitors.erase(listener->context_link);
    return _monitors.size();
}