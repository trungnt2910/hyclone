#include <cstring>

#include "haiku_errors.h"
#include "hsemaphore.h"
#include "process.h"
#include "server_servercalls.h"
#include "server_systemtime.h"
#include "server_workers.h"
#include "system.h"

const int kIntervalMicroseconds = 100 * 1000;

Semaphore::Semaphore(int pid, int count, const char* name)
    : _count(count)
{
    _info.count = count;
    strncpy(_info.name, name, sizeof(_info.name));
    _info.name[sizeof(_info.name) - 1] = '\0';
    _info.team = pid;
}

void Semaphore::Acquire(int tid, int count)
{
    while (!TryAcquire(tid, count))
    {
        server_worker_sleep(kIntervalMicroseconds);
    }
}

bool Semaphore::TryAcquire(int tid, int count)
{
    std::unique_lock<std::mutex> lock(_countLock);

    if (_count >= count)
    {
        _count -= count;
        return true;
    }

    return false;
}

bool Semaphore::TryAcquireFor(int tid, int count, int64_t timeout)
{
    while (!TryAcquire(tid, count))
    {
        if (timeout >= 0)
        {
            server_worker_sleep(kIntervalMicroseconds);
            timeout -= kIntervalMicroseconds;
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool Semaphore::TryAcquireUntil(int tid, int count, int64_t timestamp)
{
    while (!TryAcquire(tid, count))
    {
        if (server_system_time() >= timestamp)
        {
            return false;
        }
        server_worker_sleep(kIntervalMicroseconds);
    }

    return true;
}

void Semaphore::Release(int count)
{
    std::unique_lock<std::mutex> lock(_countLock);
    _count += count;
}

intptr_t server_hserver_call_create_sem(hserver_context& context, int count, const char* userName, size_t nameLength)
{
    std::string name(nameLength, '\0');

    if (context.process->ReadMemory((void*)userName, name.data(), nameLength) != nameLength)
    {
        return B_BAD_ADDRESS;
    }

    if (count < 1)
    {
        return B_BAD_VALUE;
    }

    int id;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        id = system.CreateSemaphore(context.pid, count, name.c_str());
    }

    {
        auto lock = context.process->Lock();
        context.process->AddOwningSemaphore(id);
    }

    return id;
}

intptr_t server_hserver_call_delete_sem(hserver_context& context, int id)
{
    int semId;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        auto sem = system.GetSemaphore(id).lock();

        if (!sem)
        {
            return B_BAD_SEM_ID;
        }

        if (sem->GetOwningTeam() != context.pid)
        {
            return B_BAD_SEM_ID;
        }

        system.UnregisterSemaphore(id);
        semId = sem->GetId();
    }

    {
        auto lock = context.process->Lock();
        context.process->RemoveOwningSemaphore(semId);
    }

    return B_OK;
}

intptr_t server_hserver_call_get_system_sem_count(hserver_context& context)
{
    auto& system = System::GetInstance();
    auto lock = system.Lock();

    return system.GetSemaphoreCount();
}