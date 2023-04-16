#ifndef __HYCLONE_THREAD_H__
#define __HYCLONE_THREAD_H__

#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "haiku_thread.h"

struct user_thread;

class Request;

class Thread
{
    friend class System;
private:
    haiku_thread_info _info;
    std::mutex _lock;
    std::mutex _requestLock;
    std::shared_ptr<Request> _request;
    std::atomic<bool> _suspended = false;
    std::condition_variable _blockCondition;
    std::condition_variable _sendDataCondition;
    std::condition_variable _receiveDataCondition;
    std::vector<uint8_t> _receiveData;
    user_thread* _userThreadAddress = NULL;
    int _tid;
    status_t _blockStatus;
    int _sender = -1;
    int _receiveCode = -1;
    bool _blocked = false;
    bool _registered = false;
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

    status_t SendData(std::unique_lock<std::mutex>& lock, thread_id sender, int code, std::vector<uint8_t>&& data);
    status_t ReceiveData(std::unique_lock<std::mutex>& lock, thread_id& sender, int& code, std::vector<uint8_t>& data);

    user_thread* GetUserThreadAddress() const { return _userThreadAddress; }
    void SetUserThreadAddress(user_thread* address) { _userThreadAddress = address; }

    bool IsRequesting() const { return _request != std::shared_ptr<Request>(); }
    std::shared_future<intptr_t> SendRequest(std::shared_ptr<Request> request);
    size_t RequestAck();
    status_t RequestRead(void* address);
    status_t RequestReply(intptr_t result);

    bool IsRegistered() const { return _registered; }
};

#endif // __HYCLONE_PROCESS_H__