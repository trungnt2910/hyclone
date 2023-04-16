#ifndef __HYCLONE_THREAD_H__
#define __HYCLONE_THREAD_H__

#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "haiku_thread.h"

struct user_thread;

class Request;

class Thread
{
private:
    haiku_thread_info _info;
    std::mutex _lock;
    std::mutex _requestLock;
    std::shared_ptr<Request> _request;
    std::atomic<bool> _suspended = false;
    std::condition_variable _blockCondition;
    user_thread* _userThreadAddress = NULL;
    int _tid;
    status_t _blockStatus;
    bool _blocked = false;
public:
    Thread(int pid, int tid);
    ~Thread() = default;

    std::unique_lock<std::mutex> Lock() { return std::unique_lock(_lock); }
    std::unique_lock<std::mutex> RequestLock() { return std::unique_lock(_requestLock); }

    int GetTid() const { return _tid; }
    const haiku_thread_info& GetInfo() const { return _info; }
    haiku_thread_info& GetInfo() { return _info; }
    void SuspendSelf();
    void SetSuspended(bool suspended);
    void Resume();
    void WaitForResume();

    status_t Block(std::unique_lock<std::mutex>& lock, uint32 flags, bigtime_t timeout);
    status_t Unblock(status_t status);

    user_thread* GetUserThreadAddress() const { return _userThreadAddress; }
    void SetUserThreadAddress(user_thread* address) { _userThreadAddress = address; }

    bool IsRequesting() const { return _request != std::shared_ptr<Request>(); }
    std::shared_future<intptr_t> SendRequest(std::shared_ptr<Request> request);
    size_t RequestAck();
    status_t RequestRead(void* address);
    status_t RequestReply(intptr_t result);
};

#endif // __HYCLONE_PROCESS_H__