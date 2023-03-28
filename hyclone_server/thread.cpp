#include <cstring>
#include <iostream>
#include <memory>
#include <sched.h>

#include "haiku_errors.h"
#include "haiku_thread.h"
#include "server_native.h"
#include "server_servercalls.h"
#include "system.h"
#include "thread.h"

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
    _suspended.wait(true);
}

void Thread::SetSuspended(bool suspended)
{
    _suspended = suspended;
}

void Thread::Resume()
{
    _suspended = false;
    _suspended.notify_all();
}

void Thread::WaitForResume()
{
    _suspended.wait(true);
}

intptr_t server_hserver_call_get_thread_info(hserver_context& context, int thread_id, void* userThreadInfo)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Thread> thread;

    {
        auto lock = system.Lock();
        thread = system.GetThread(thread_id).lock();
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

intptr_t server_hserver_call_rename_thread(hserver_context& context, int thread_id, const char* userNewName, size_t len)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Thread> thread;

    {
        auto lock = system.Lock();
        thread = system.GetThread(thread_id).lock();
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

    return B_OK;
}

intptr_t server_hserver_call_set_thread_priority(hserver_context& context, int thread_id, int newPriority)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Thread> thread;

    {
        auto lock = system.Lock();
        thread = system.GetThread(thread_id).lock();
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

    if (sched_setscheduler(thread_id, policy, &param) == -1)
    {
        return B_NOT_ALLOWED;
    }

    thread->GetInfo().priority = newPriority;

    return B_OK;
}

intptr_t server_hserver_call_suspend_thread(hserver_context& context, int thread_id)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Thread> thread;

    {
        auto lock = system.Lock();
        thread = system.GetThread(thread_id).lock();
    }

    if (!thread)
    {
        return B_BAD_THREAD_ID;
    }

    if (thread_id != context.tid)
    {
        std::cerr << "suspend_thread: " << context.pid << " " << context.tid << " " << thread_id << std::endl;
        std::cerr << "Suspending another thread is currently not supported in Hyclone." << std::endl;
        return HAIKU_POSIX_ENOSYS;
    }

    thread->GetInfo().state = B_THREAD_SUSPENDED;
    thread->SuspendSelf();

    return B_OK;
}

intptr_t server_hserver_call_resume_thread(hserver_context& context, int thread_id)
{
    auto& system = System::GetInstance();

    std::shared_ptr<Thread> thread;

    {
        auto lock = system.Lock();
        thread = system.GetThread(thread_id).lock();
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