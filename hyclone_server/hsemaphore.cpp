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

int Semaphore::Acquire(int tid, int count)
{
    std::unique_lock<std::mutex> lock(_countLock);
    if (_count >= count)
    {
        _count -= count;
        return B_OK;
    }

    ++_waitCount;

    server_worker_run_wait([&]()
    {
        _countCondVar.wait(lock, [&]()
        {
            return !_registered || _count >= count;
        });
    });

    --_waitCount;

    if (!_registered)
    {
        return B_BAD_SEM_ID;
    }

    _count -= count;

    return B_OK;
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
    std::unique_lock<std::mutex> lock(_countLock);

    ++_waitCount;

    server_worker_run_wait([&]()
    {
        _countCondVar.wait_for(lock, std::chrono::microseconds(timeout), [&]()
        {
            return !_registered || _count >= count;
        });
    });

    --_waitCount;

    if (!_registered)
    {
        return B_BAD_SEM_ID;
    }

    if (_count < count)
    {
        return timeout == 0 ? B_WOULD_BLOCK : B_TIMED_OUT;
    }

    _count -= count;
    return B_OK;
}

int Semaphore::TryAcquireUntil(int tid, int count, int64_t timestamp)
{
    std::unique_lock<std::mutex> lock(_countLock);

    if (_count >= count)
    {
        _count -= count;
        return B_OK;
    }

    ++_waitCount;

    server_worker_run_wait([&]()
    {
        _countCondVar.wait_until(lock,
            std::chrono::steady_clock::time_point(std::chrono::microseconds(timestamp)), [&]()
        {
            return !_registered || _count >= count;
        });
    });

    --_waitCount;

    if (!_registered)
    {
        return B_BAD_SEM_ID;
    }

    if (_count < count)
    {
        return B_TIMED_OUT;
    }

    _count -= count;
    return B_OK;
}

void Semaphore::Release(int count)
{
    std::unique_lock<std::mutex> lock(_countLock);
    _count += count;
    _countCondVar.notify_all();
}

int Semaphore::GetSemCount()
{
    std::unique_lock<std::mutex> lock(_countLock);

    if (_count >= 0)
    {
        return _count;
    }

    return -_waitCount;
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
    std::shared_ptr<Semaphore> sem;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        sem = system.GetSemaphore(id).lock();
    }

    if (!sem)
    {
        return B_BAD_SEM_ID;
    }

    return sem->Acquire(context.tid, 1);
}

intptr_t server_hserver_call_acquire_sem_etc(hserver_context& context, int id, unsigned int count,
    unsigned int flags, unsigned long long timeout)
{
    std::shared_ptr<Semaphore> sem;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        sem = system.GetSemaphore(id).lock();
    }

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

    if (flags & B_RELATIVE_TIMEOUT && timeout != B_INFINITE_TIMEOUT)
    {
        return sem->TryAcquireFor(context.tid, count, timeout);
    }
    else if (flags & B_ABSOLUTE_TIMEOUT && timeout != B_INFINITE_TIMEOUT)
    {
        return sem->TryAcquireUntil(context.tid, count, timeout);
    }
    else
    {
        return sem->Acquire(context.tid, count);
    }
}

intptr_t server_hserver_call_release_sem(hserver_context& context, int id)
{
    std::shared_ptr<Semaphore> sem;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        sem = system.GetSemaphore(id).lock();
    }

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
    std::shared_ptr<Semaphore> sem;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        sem = system.GetSemaphore(id).lock();
    }

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

intptr_t server_hserver_call_get_sem_count(hserver_context& context, int id, int* userThreadCount)
{
    std::shared_ptr<Semaphore> sem;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        sem = system.GetSemaphore(id).lock();
    }

    if (!sem)
    {
        return B_BAD_SEM_ID;
    }

    int threadCount = sem->GetSemCount();

    if (context.process->WriteMemory(userThreadCount, &threadCount, sizeof(threadCount)) != sizeof(threadCount))
    {
        return B_BAD_ADDRESS;
    }

    return B_OK;
}

intptr_t server_hserver_call_get_system_sem_count(hserver_context& context)
{
    auto& system = System::GetInstance();
    auto lock = system.Lock();

    return system.GetSemaphoreCount();
}