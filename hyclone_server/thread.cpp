#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <sched.h>

#include "haiku_errors.h"
#include "haiku_thread.h"
#include "process.h"
#include "server_native.h"
#include "server_requests.h"
#include "server_servercalls.h"
#include "server_systemnotification.h"
#include "server_workers.h"
#include "system.h"
#include "thread.h"
#include "user_thread_defs.h"

#define B_IDLE_PRIORITY              0
#define B_LOWEST_ACTIVE_PRIORITY     1
#define B_LOW_PRIORITY               5
#define B_NORMAL_PRIORITY            10
#define B_DISPLAY_PRIORITY           15
#define B_URGENT_DISPLAY_PRIORITY    20
#define B_REAL_TIME_DISPLAY_PRIORITY 100
#define B_URGENT_PRIORITY            110
#define B_REAL_TIME_PRIORITY         120

Thread::Thread(int pid, int tid) : _tid(tid)
{
    memset(&_info, 0, sizeof(_info));
    _info.thread = tid;
    _info.team = pid;
}

void Thread::SuspendSelf()
{
    _suspended = true;
    server_worker_run_wait([&]()
    {
        _suspended.wait(true);
    });
}

void Thread::SetSuspended(bool suspended)
{
    _suspended = suspended;
    _info.state = suspended ? B_THREAD_SUSPENDED : B_THREAD_RUNNING;
}

void Thread::Resume()
{
    _suspended = false;
    _info.state = B_THREAD_RUNNING;
    _suspended.notify_all();
}

void Thread::WaitForResume()
{
    server_worker_run_wait([&]()
    {
        _suspended.wait(true);
    });
}

status_t Thread::Block(std::unique_lock<std::mutex>& lock, uint32 flags, bigtime_t timeout)
{
    // check, if already done
    status_t waitStatus;
    if (server_read_process_memory(_info.team, &_userThreadAddress->wait_status,
        &waitStatus, sizeof(waitStatus)) != sizeof(waitStatus))
    {
        return B_BAD_ADDRESS;
    }
    if (waitStatus <= 0)
    {
        return waitStatus;
    }

    // nope, so wait
    _blocked = true;

    bool useTimeout = (flags & (B_ABSOLUTE_TIMEOUT | B_RELATIVE_TIMEOUT)) &&
        timeout != B_INFINITE_TIMEOUT;

    server_worker_run_wait([&]()
    {
        if (!useTimeout)
        {
            _blockCondition.wait(lock, [&]()
            {
                return !_blocked;
            });
        }
        else if (flags & B_RELATIVE_TIMEOUT)
        {
            _blockCondition.wait_for(lock, std::chrono::microseconds(timeout), [&]()
            {
                return !_blocked;
            });
        }
        else // if (flags & B_ABSOLUTE_TIMEOUT)
        {
            if (flags & B_TIMEOUT_REAL_TIME_BASE)
            {
                _blockCondition.wait_until(lock,
                    std::chrono::system_clock::time_point(std::chrono::microseconds(timeout)), [&]()
                {
                    return !_blocked;
                });
            }
            else
            {
                _blockCondition.wait_until(lock,
                    std::chrono::steady_clock::time_point(std::chrono::microseconds(timeout)), [&]()
                {
                    return !_blocked;
                });
            }
        }
    });

    if (_blocked)
    {
        _blockStatus = B_TIMED_OUT;
    }

    _blocked = false;

    if (server_write_process_memory(_info.team, &_userThreadAddress->wait_status,
        &_blockStatus, sizeof(_blockStatus)) != sizeof(_blockStatus))
    {
        return B_BAD_ADDRESS;
    }

    return _blockStatus;
}

status_t Thread::Unblock(status_t status)
{
    if (server_write_process_memory(_info.team, &_userThreadAddress->wait_status,
        &status, sizeof(status)) != sizeof(status))
    {
        return B_BAD_ADDRESS;
    }

    _blockStatus = status;
    _blocked = false;

    _blockCondition.notify_all();

    return B_OK;
}

status_t Thread::SendData(std::unique_lock<std::mutex>& lock, thread_id sender, int code, std::vector<uint8_t>&& data)
{
    if (_sender != -1)
    {
        server_worker_run_wait([&]()
        {
            _sendDataCondition.wait(lock, [&]()
            {
                return !_registered || _sender == -1;
            });
        });
    }

    if (!_registered)
    {
        return B_BAD_THREAD_ID;
    }

    _sender = sender;
    _receiveCode = code;
    _receiveData = std::move(data);

    _receiveDataCondition.notify_all();

    return B_OK;
}

status_t Thread::ReceiveData(std::unique_lock<std::mutex>& lock, thread_id& sender, int& code, std::vector<uint8_t>& data)
{
    if (_sender == -1)
    {
        server_worker_run_wait([&]()
        {
            _receiveDataCondition.wait(lock, [&]()
            {
                return !_registered || _sender != -1;
            });
        });
    }

    if (!_registered)
    {
        return B_BAD_THREAD_ID;
    }

    sender = _sender;
    code = _receiveCode;
    data = std::move(_receiveData);


    _sender = -1;

    _sendDataCondition.notify_all();

    return B_OK;
}

std::shared_future<intptr_t> Thread::SendRequest(std::shared_ptr<Request> request)
{
    assert(!IsRequesting());
    _request = request;
    server_send_request(_info.team, _info.thread);

    return request->Result();
}

size_t Thread::RequestAck()
{
    return _request->Size();
}

status_t Thread::RequestRead(void* address)
{
    status_t status = _request->Relocate(address);
    if (status != B_OK)
    {
        return status;
    }

    if (server_write_process_memory(_info.team, address, _request->Data(), _request->Size()) != _request->Size())
    {
        return B_BAD_ADDRESS;
    }

    return B_OK;
}

status_t Thread::RequestReply(intptr_t result)
{
    status_t status = _request->Reply(result);

    if (status != B_OK)
    {
        return status;
    }

    _request = std::shared_ptr<Request>();
    return B_OK;
}

intptr_t server_hserver_call_register_thread_info(hserver_context& context, void* userThreadInfo)
{
    auto& system = System::GetInstance();

    haiku_thread_info info;
    if (server_read_process_memory(context.pid, userThreadInfo, &info, sizeof(haiku_thread_info))
        != sizeof(haiku_thread_info))
    {
        return B_BAD_ADDRESS;
    }

    if (context.tid != info.thread)
    {
        return B_NOT_ALLOWED;
    }

    if (context.pid != info.team)
    {
        return B_NOT_ALLOWED;
    }

    std::shared_ptr<Thread> thread;
    {
        auto lock = system.Lock();
        thread = system.GetThread(context.tid).lock();
    }

    if (!thread)
    {
        return B_BAD_THREAD_ID;
    }

    // No locking required, since this thread is the only one modifying this.
    haiku_thread_info& threadInfo = thread->GetInfo();
    strncpy(threadInfo.name, info.name, sizeof(threadInfo.name));
    threadInfo.priority = info.priority;
    threadInfo.sem = info.sem;
    threadInfo.stack_base = info.stack_base;
    threadInfo.stack_end = info.stack_end;
    threadInfo.state = info.state;

    // Used when registering a newly spawned thread.
    if (threadInfo.state == B_THREAD_SUSPENDED)
    {
        thread->SetSuspended(true);
    }

    return B_OK;
}

intptr_t server_hserver_call_get_thread_info(hserver_context& context, int threadId, void* userThreadInfo)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Thread> thread;

    {
        auto lock = system.Lock();
        thread = system.GetThread(threadId).lock();
    }

    if (!thread)
    {
        return B_BAD_THREAD_ID;
    }

    haiku_thread_info info = thread->GetInfo();
    server_fill_thread_info(&info);

    if (server_write_process_memory(context.pid, userThreadInfo, &info, sizeof(haiku_thread_info))
        != sizeof(haiku_thread_info))
    {
        return B_BAD_ADDRESS;
    }

    return B_OK;
}

intptr_t server_hserver_call_get_next_thread_info(hserver_context& context, int team, int* userCookie,
    void* userThreadInfo)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Process> process;

    if (team == B_CURRENT_TEAM)
    {
        process = context.process;
    }
    else
    {
        auto lock = system.Lock();
        process = system.GetProcess(team).lock();
    }

    if (!process)
    {
        return B_BAD_TEAM_ID;
    }

    int cookie = 0;

    {
        auto lock = context.process->Lock();
        if (context.process->ReadMemory(userCookie, &cookie, sizeof(int)) != sizeof(int))
        {
            return B_BAD_ADDRESS;
        }
    }

    if (cookie == -1)
    {
        return B_BAD_VALUE;
    }

    std::shared_ptr<Thread> thread;

    {
        auto lock = process->Lock();
        cookie = process->NextThreadId(cookie);

        if (cookie != -1)
        {
            thread = process->GetThread(cookie).lock();

            if (!thread)
            {
                return B_BAD_THREAD_ID;
            }
        }
    }

    haiku_thread_info info;

    if (thread)
    {
        auto lock = thread->Lock();
        info = thread->GetInfo();
        server_fill_thread_info(&info);
    }

    {
        auto lock = context.process->Lock();
        if (thread)
        {
            if (context.process->WriteMemory(userThreadInfo, &info, sizeof(haiku_thread_info))
                != sizeof(haiku_thread_info))
            {
                return B_BAD_ADDRESS;
            }
        }

        if (context.process->WriteMemory(userCookie, &cookie, sizeof(int)) != sizeof(int))
        {
            return B_BAD_ADDRESS;
        }
    }

    return thread ? B_OK : B_BAD_VALUE;
}

intptr_t server_hserver_call_rename_thread(hserver_context& context, int threadId, const char* userNewName, size_t len)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Thread> thread;

    {
        auto lock = system.Lock();
        thread = system.GetThread(threadId).lock();
    }

    if (!thread)
    {
        return B_BAD_THREAD_ID;
    }

    {
        auto lock = thread->Lock();
        haiku_thread_info& info = thread->GetInfo();

        len = std::min(len, sizeof(info.name) - 1);
        if (server_read_process_memory(context.pid, (void*)userNewName, info.name, len) != len)
        {
            return B_BAD_ADDRESS;
        }

        info.name[len] = '\0';
    }

    System::GetInstance().GetThreadNotificationService().Notify(THREAD_NAME_CHANGED, thread);

    return B_OK;
}

intptr_t server_hserver_call_set_thread_priority(hserver_context& context, int threadId, int newPriority)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Thread> thread;

    {
        auto lock = system.Lock();
        thread = system.GetThread(threadId).lock();
    }

    if (!thread)
    {
        return B_BAD_THREAD_ID;
    }

    auto threadLock = thread->Lock();

    if (newPriority < B_IDLE_PRIORITY || newPriority > B_REAL_TIME_PRIORITY)
    {
        return B_BAD_VALUE;
    }

    sched_param param;
    int policy;

    if (newPriority <= B_URGENT_DISPLAY_PRIORITY)
    {
        param.sched_priority = 0;
        policy = SCHED_OTHER;
    }
    else
    {
        param.sched_priority = newPriority - B_URGENT_DISPLAY_PRIORITY;
        policy = SCHED_RR;
    }

    if (sched_setscheduler(threadId, policy, &param) == -1)
    {
        return B_NOT_ALLOWED;
    }

    thread->GetInfo().priority = newPriority;

    return B_OK;
}

intptr_t server_hserver_call_suspend_thread(hserver_context& context, int threadId)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Thread> thread;

    {
        auto lock = system.Lock();
        thread = system.GetThread(threadId).lock();
    }

    if (!thread)
    {
        return B_BAD_THREAD_ID;
    }

    // Currently, this call is used internally in HyClone to force a thread to suspend itself.
    // After issuing this call, it will have to call wait_for_resume when it is ready to resume.
    //
    // When we support suspending other threads, this call should issue a "request" to the target
    // thread. In the request handler, that target thread should also call wait_for_resume when it
    // is ready.
    if (threadId != context.tid)
    {
        std::cerr << "suspend_thread: " << context.pid << " " << context.tid << " " << threadId << std::endl;
        std::cerr << "Suspending another thread is currently not supported in HyClone." << std::endl;
        return HAIKU_POSIX_ENOSYS;
    }

    thread->GetInfo().state = B_THREAD_SUSPENDED;

    return B_OK;
}

intptr_t server_hserver_call_resume_thread(hserver_context& context, int threadId)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Thread> thread;

    {
        auto lock = system.Lock();
        thread = system.GetThread(threadId).lock();
    }

    if (!thread)
    {
        return B_BAD_THREAD_ID;
    }

    thread->Resume();
    thread->GetInfo().state = B_THREAD_RUNNING;

    return B_OK;
}

intptr_t server_hserver_call_wait_for_resume(hserver_context& context)
{
    context.thread->WaitForResume();

    return B_OK;
}

intptr_t server_hserver_call_register_user_thread(hserver_context& context, void* address)
{
    context.thread->SetUserThreadAddress((user_thread*)address);

    return B_OK;
}

intptr_t server_hserver_call_block_thread(hserver_context& context, int flags, unsigned long long timeout)
{
    auto lock = context.thread->Lock();
    return context.thread->Block(lock, flags, timeout);
}

intptr_t server_hserver_call_unblock_thread(hserver_context& context, int threadId, int status)
{
    std::shared_ptr<Thread> thread;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        thread = system.GetThread(threadId).lock();
    }

    if (!thread)
    {
        return B_BAD_THREAD_ID;
    }

    {
        auto lock = thread->Lock();
        return thread->Unblock(status);
    }
}

intptr_t server_hserver_call_send_data(hserver_context& context, int threadId, int code, const void* data, size_t len)
{
    std::shared_ptr<Thread> thread;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        thread = system.GetThread(threadId).lock();
    }

    if (!thread)
    {
        return B_BAD_THREAD_ID;
    }

    std::vector<uint8_t> buffer(len);

    {
        auto lock = context.process->Lock();
        if (context.process->ReadMemory((void*)data, buffer.data(), len) != len)
        {
            return B_BAD_ADDRESS;
        }
    }

    {
        auto lock = thread->Lock();
        return thread->SendData(lock, context.tid, code, std::move(buffer));
    }
}

intptr_t server_hserver_call_receive_data(hserver_context& context, int* userSender, void* userData, size_t len)
{
    thread_id sender;
    int code;

    std::vector<uint8_t> data;

    {
        auto lock = context.thread->Lock();

        status_t status = context.thread->ReceiveData(lock, sender, code, data);

        if (status != B_OK)
        {
            return status;
        }
    }

    len = std::min(len, data.size());

    {
        auto lock = context.process->Lock();
        if (context.process->WriteMemory(userData, data.data(), len) != len)
        {
            return B_BAD_ADDRESS;
        }

        if (context.process->WriteMemory(userSender, &sender, sizeof(sender)) != sizeof(sender))
        {
            return B_BAD_ADDRESS;
        }
    }

    return code;
}

intptr_t server_hserver_call_request_ack(hserver_context& context, int pid, int tid)
{
    auto lock = context.thread->Lock();

    if (!context.thread->IsRequesting())
    {
        return B_BAD_VALUE;
    }

    ssize_t status = context.thread->RequestAck();

    if (status < 0)
    {
        return status;
    }

    {
        auto& system = System::GetInstance();
        auto systemLock = system.Lock();

        system.RegisterConnection(context.conn_id, Connection(pid, tid, false));
    }

    return status;
}

intptr_t server_hserver_call_request_read(hserver_context& context, void* userData)
{
    auto lock = context.thread->Lock();

    if (!context.thread->IsRequesting())
    {
        return B_BAD_VALUE;
    }

    return context.thread->RequestRead(userData);
}

intptr_t server_hserver_call_request_reply(hserver_context& context, intptr_t reply)
{
    auto lock = context.thread->Lock();

    if (!context.thread->IsRequesting())
    {
        return B_BAD_VALUE;
    }

    return context.thread->RequestReply(reply);
}