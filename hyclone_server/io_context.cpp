#include "io_context.h"
#include "server_nodemonitor.h"
#include "server_vfs.h"
#include "system.h"

IoContext::IoContext(const IoContext& other)
{
    _maxMonitors = other._maxMonitors;

    // Monitors are not copied.
}

IoContext::~IoContext()
{
    auto& vfsService = System::GetInstance().GetVfsService();
    auto lock = vfsService.Lock();
    for (auto& listener : _monitors)
    {
        if (listener && listener->monitor
            && listener->monitor->node != (haiku_ino_t)-1)
        {
            vfsService.RemoveMonitor(listener->monitor->device, listener->monitor->node);
        }
    }
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
    if (listener && listener->monitor
        && listener->monitor->node != (haiku_ino_t)-1)
    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();
        vfsService.AddMonitor(listener->monitor->device, listener->monitor->node);
    }
    return _monitors.size();
}

size_t IoContext::RemoveMonitor(const std::shared_ptr<monitor_listener>& listener)
{
    _monitors.erase(listener->context_link);
    if (listener && listener->monitor
        && listener->monitor->node != (haiku_ino_t)-1)
    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();
        vfsService.RemoveMonitor(listener->monitor->device, listener->monitor->node);
    }
    return _monitors.size();
}