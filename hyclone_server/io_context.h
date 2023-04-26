#ifndef __HYCLONE_IO_CONTEXT_H__
#define __HYCLONE_IO_CONTEXT_H__

#define DEFAULT_NODE_MONITORS 4096

#include <list>
#include <memory>

class monitor_listener;

class IoContext
{
private:
    unsigned int _maxMonitors = DEFAULT_NODE_MONITORS;
    std::list<std::shared_ptr<monitor_listener>> _monitors;
public:
    IoContext() = default;
    IoContext(const IoContext& other);
    IoContext& operator=(const IoContext& other);
    ~IoContext();

    unsigned int MaxMonitors() const { return _maxMonitors; }
    unsigned int NumMonitors() const;
    size_t AddMonitor(const std::shared_ptr<monitor_listener>& listener);
    size_t RemoveMonitor(const std::shared_ptr<monitor_listener>& listener);
    std::list<std::shared_ptr<monitor_listener>>& GetMonitors() { return _monitors; }
};

#endif // __HYCLONE_IO_CONTEXT_H__