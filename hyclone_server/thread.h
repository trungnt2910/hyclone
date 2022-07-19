#ifndef __HYCLONE_THREAD_H__
#define __HYCLONE_THREAD_H__

#include <memory>
#include <mutex>
#include <unordered_map>

class Thread
{
private:
    int _tid;

    std::mutex _lock;
public:
    Thread(int tid): _tid(tid) {}
    ~Thread() = default;

    std::unique_lock<std::mutex> Lock() { return std::unique_lock(_lock); }

    int GetTid() const { return _tid; }
};

#endif // __HYCLONE_PROCESS_H__