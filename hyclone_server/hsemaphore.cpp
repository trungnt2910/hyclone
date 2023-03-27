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
    while (TryAcquire(tid, count))
    {
        server_worker_sleep(kIntervalMicroseconds);
    }
}

int Semaphore::TryAcquire(int tid, int count)
{
    std::unique_lock<std::mutex> lock(_countLock);

    if (_count >= count)
    {
        _count -= count;
        return B_OK;
    }

    return B_WOULD_BLOCK;
}

int Semaphore::TryAcquireFor(int tid, int count, int64_t timeout)
{
    if (timeout == 0)
    {
        return TryAcquire(tid, count);
    }

    status_t status = B_OK;

    while ((status = TryAcquire(tid, count)) == B_WOULD_BLOCK)
    {
        if (timeout <= 0)
        {
            return B_TIMED_OUT;
        }
        if (!_registered)
        {
            return B_BAD_SEM_ID;
        }
        server_worker_sleep(std::min(timeout, (int64_t)kIntervalMicroseconds));
        timeout -= kIntervalMicroseconds;
    }

    return status;
}

int Semaphore::TryAcquireUntil(int tid, int count, int64_t timestamp)
{
    status_t status = B_OK;

    while ((status = TryAcquire(tid, count)) == B_WOULD_BLOCK)
    {
        if (server_system_time() >= timestamp)
        {
            return B_TIMED_OUT;
        }
        if (!_registered)
        {
            return B_BAD_SEM_ID;
        }
        server_worker_sleep(std::min(timestamp - server_system_time(), (int64_t)kIntervalMicroseconds));
    }

    return status;
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

    if (count < 0)
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

intptr_t server_hserver_call_acquire_sem(hserver_context& context, int id)
{
    auto& system = System::GetInstance();
    auto lock = system.Lock();

    auto sem = system.GetSemaphore(id).lock();

    if (!sem)
    {
        return B_BAD_SEM_ID;
    }

    sem->Acquire(context.tid, 1);

    return B_OK;
}

intptr_t server_hserver_call_acquire_sem_etc(hserver_context& context, int id, unsigned int count,
    unsigned int flags, unsigned long long timeout)
{
    auto& system = System::GetInstance();
    auto lock = system.Lock();

    auto sem = system.GetSemaphore(id).lock();

    if (!sem)
    {
        return B_BAD_SEM_ID;
    }

    if (count < 1)
    {
        return B_BAD_VALUE;
    }

    if (flags & ~(B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT))
    {
        return B_BAD_VALUE;
    }

    if ((flags & (B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT))
        == (B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT))
    {
        return B_BAD_VALUE;
    }

    if (flags & B_RELATIVE_TIMEOUT)
    {
        return sem->TryAcquireFor(context.tid, count, timeout);
    }
    else if (flags & B_ABSOLUTE_TIMEOUT)
    {
        return sem->TryAcquireUntil(context.tid, count, timeout);
    }
    else
    {
        sem->Acquire(context.tid, count);
        return B_OK;
    }
}

intptr_t server_hserver_call_release_sem(hserver_context& context, int id)
{
    auto& system = System::GetInstance();
    auto lock = system.Lock();

    auto sem = system.GetSemaphore(id).lock();

    if (!sem)
    {
        return B_BAD_SEM_ID;
    }

    sem->Release(1);

    return B_OK;
}

intptr_t server_hserver_call_release_sem_etc(hserver_context& context, int id, unsigned int count,
    unsigned int flags)
{
    auto& system = System::GetInstance();
    auto lock = system.Lock();

    auto sem = system.GetSemaphore(id).lock();

    if (!sem)
    {
        return B_BAD_SEM_ID;
    }

    if (flags & ~(B_DO_NOT_RESCHEDULE))
    {
        return B_BAD_VALUE;
    }

    sem->Release(count);

    return B_OK;
}

intptr_t server_hserver_call_get_system_sem_count(hserver_context& context)
{
    auto& system = System::GetInstance();
    auto lock = system.Lock();

    return system.GetSemaphoreCount();
}