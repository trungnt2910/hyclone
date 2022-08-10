#include <atomic>
#include <mutex>
#include <unordered_map>

#include "BeDefs.h"
#include "haiku_errors.h"
#include "loader_mutex.h"
#include "user_mutex_defs.h"

struct mutex_info
{
    int waiter_count;
};

std::timed_mutex sMutexTableLock;
std::unordered_map<int32_t*, std::pair<std::timed_mutex, mutex_info>> sMutexTable;

static bool LockTimedMutex(std::timed_mutex& mutex, int64_t timeout, uint32_t flags);
static int32_t atomic_or(int32_t* a, int32_t b);
static int32_t atomic_and(int32_t* a, int32_t b);

int loader_mutex_lock(int32_t* mutex, const char* name, uint32_t flags, int64_t timeout)
{
    std::timed_mutex* cppMutex;
    {
        if (!LockTimedMutex(sMutexTableLock, timeout, flags))
        {
            return B_WOULD_BLOCK;
        }
        auto& [tempMutex, info] = sMutexTable[mutex];
        // This thing is only accessible in the mutex
        // subsystem and is therefore safe to increment.
        ++info.waiter_count;
        atomic_or(mutex, B_USER_MUTEX_WAITING);
        cppMutex = &tempMutex;
        sMutexTableLock.unlock();
    }

    bool locked = LockTimedMutex(*cppMutex, timeout, flags);

    {
        // This section is required. Therefore, it does not respect the timeout parameter.
        sMutexTableLock.lock();
        auto& [tempMutex, info] = sMutexTable[mutex];
        --info.waiter_count;
        if (info.waiter_count == 0)
        {
            atomic_and(mutex, ~B_USER_MUTEX_WAITING);
        }
        if (locked)
        {
            atomic_or(mutex, B_USER_MUTEX_LOCKED);
        }
        sMutexTableLock.unlock();
    }

    return locked ? B_OK : B_WOULD_BLOCK;
}

int loader_mutex_unlock(int32* mutex, uint32 flags)
{
    // TODO: Support B_USER_MUTEX_UNBLOCK_ALL
    {
        sMutexTableLock.lock();
        bool shouldErase = false;
        {
            auto& [cppMutex, info] = sMutexTable[mutex];
            if (info.waiter_count == 0)
            {
                atomic_and(mutex, ~B_USER_MUTEX_LOCKED);
                shouldErase = true;
            }
            else
            {
                atomic_or(mutex, B_USER_MUTEX_LOCKED | B_USER_MUTEX_WAITING);
            }
            cppMutex.unlock();
        }
        if (shouldErase)
        {
            sMutexTable.erase(mutex);
        }
        sMutexTableLock.unlock();
    }

    return B_OK;
}

int loader_mutex_switch_lock(int32_t* fromMutex, int32_t* toMutex,
    const char* name, uint32_t flags, int64_t timeout)
{
    std::timed_mutex* newCppMutex;
    {
        if (!LockTimedMutex(sMutexTableLock, timeout, flags))
        {
            return B_WOULD_BLOCK;
        }
        auto& [oldTempMutex, oldInfo] = sMutexTable[fromMutex];
        auto& [newTempMutex, newInfo] = sMutexTable[toMutex];

        if (oldInfo.waiter_count == 0)
        {
            atomic_and(fromMutex, ~B_USER_MUTEX_LOCKED);
            sMutexTable.erase(fromMutex);
        }
        else
        {
            atomic_or(fromMutex, B_USER_MUTEX_LOCKED | B_USER_MUTEX_WAITING);
        }

        ++newInfo.waiter_count;
        atomic_or(toMutex, B_USER_MUTEX_WAITING);
        newCppMutex = &newTempMutex;

        oldTempMutex.unlock();
        sMutexTableLock.unlock();
    }

    bool locked = LockTimedMutex(*newCppMutex, timeout, flags);

    {
        sMutexTableLock.lock();
        auto& [tempMutex, info] = sMutexTable[toMutex];
        --info.waiter_count;
        if (info.waiter_count == 0)
        {
            atomic_and(toMutex, ~B_USER_MUTEX_WAITING);
        }
        if (locked)
        {
            atomic_or(toMutex, B_USER_MUTEX_LOCKED);
        }
        sMutexTableLock.unlock();
    }

    return locked ? B_OK : B_WOULD_BLOCK;
}

static bool LockTimedMutex(std::timed_mutex& mutex, int64_t timeout, uint32_t flags)
{
    bool absolute = flags & B_ABSOLUTE_TIMEOUT;
    bool relative = flags & B_RELATIVE_TIMEOUT;

    if (timeout == B_INFINITE_TIMEOUT || (!absolute && !relative))
    {
        mutex.lock();
        return true;
    }
    else if (absolute)
    {
        return mutex.try_lock_until(
            std::chrono::system_clock::time_point(std::chrono::microseconds(timeout)));
    }
    else if (relative)
    {
        return mutex.try_lock_for(std::chrono::microseconds(timeout));
    }

    return false;
}

static int32_t atomic_or(int32_t* a, int32_t b)
{
    static_assert(sizeof(std::atomic<int32_t>) == sizeof(int32_t), "Can't use this hack on this platform.");
    auto atomic_a = (std::atomic<int32_t>*)a;
    return atomic_a->fetch_or(b);
}

static int32_t atomic_and(int32_t* a, int32_t b)
{
    static_assert(sizeof(std::atomic<int32_t>) == sizeof(int32_t), "Can't use this hack on this platform.");
    auto atomic_a = (std::atomic<int32_t>*)a;
    return atomic_a->fetch_and(b);
}