#ifndef __HYCLONE_THREAD_H__
#define __HYCLONE_THREAD_H__

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "haiku_thread.h"

class Thread
{
private:
    haiku_thread_info _info;
    std::mutex _lock;
    std::atomic<bool> _suspended = false;
    int _tid;
public:
    Thread(int pid, int tid);
    ~Thread() = default;

    std::unique_lock<std::mutex> Lock() { return std::unique_lock(_lock); }

    int GetTid() const { return _tid; }
    const haiku_thread_info& GetInfo() const { return _info; }
    haiku_thread_info& GetInfo() { return _info; }
    void SuspendSelf();
    void SetSuspended(bool suspended);
    void Resume();
    void WaitForResume();
};

#endif // __HYCLONE_PROCESS_H__