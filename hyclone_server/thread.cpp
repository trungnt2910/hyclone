#include <memory>

#include "haiku_errors.h"
#include "haiku_thread.h"
#include "server_native.h"
#include "server_servercalls.h"
#include "system.h"
#include "thread.h"

Thread::Thread(int pid, int tid) : _tid(tid)
{
    _info.thread = tid;
    _info.team = pid;
    _info.name[0] = '\0';
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