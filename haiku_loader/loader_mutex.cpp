#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "BeDefs.h"
#include "haiku_errors.h"
#include "loader_mutex.h"
#include "user_mutex_defs.h"

// Logic based on https://xref.landonf.org/source/xref/haiku/src/system/kernel/locks/user_mutex.cpp

struct mutex_info
{
    bool locked;
    const std::shared_ptr<std::condition_variable> condition;
    std::weak_ptr<std::list<std::shared_ptr<mutex_info>>> otherWaiters;

    mutex_info() : locked(false), condition(std::make_unique<std::condition_variable>()) {}
    bool operator==(const mutex_info &other) const
    {
        return locked == other.locked && condition == other.condition &&
            !otherWaiters.expired() && !other.otherWaiters.expired() &&
            otherWaiters.lock() == other.otherWaiters.lock();
    }
};

std::mutex sMutexTableLock;
std::unordered_map<int32_t *, std::shared_ptr<std::list<std::shared_ptr<mutex_info>>>> sMutexTable;

static int32_t atomic_or(int32_t *a, int32_t b);
static int32_t atomic_and(int32_t *a, int32_t b);
static int loader_mutex_lock_locked(int32_t *mutex, const char *name, uint32_t flags, int64_t timeout,
    std::unique_lock<std::mutex> &lock);
static void loader_mutex_unblock_locked(int32_t *mutex, uint32_t flags);
static int loader_mutex_wait_locked(int32_t *mutex, const char *name, uint32_t flags, int64_t timeout,
    std::unique_lock<std::mutex> &lock, bool &lastWaiter);
static void add_loader_mutex_info(int32_t *mutex, std::shared_ptr<mutex_info>  info);
static bool remove_loader_mutex_info(int32_t *mutex, std::shared_ptr<mutex_info> info);
static int loader_condvar_wait(std::condition_variable &cond, std::unique_lock<std::mutex> &lock,
    bool& stopWaiting, int64_t timeout, uint32_t flags);

int loader_mutex_lock(int32_t *mutex, const char *name, uint32_t flags, int64_t timeout)
{
    if (mutex == NULL || ((intptr_t)mutex) % 4 != 0)
    {
        return B_BAD_ADDRESS;
    }

    int error;

    {
        auto lock = std::unique_lock<std::mutex>(sMutexTableLock);

        error = loader_mutex_lock_locked(mutex, name, flags, timeout, lock);
    }

    return error;
}

int loader_mutex_unblock(int32_t *mutex, uint32 flags)
{
    if (mutex == NULL || ((intptr_t)mutex) % 4 != 0)
    {
        return B_BAD_ADDRESS;
    }

    {
        auto lock = std::unique_lock<std::mutex>(sMutexTableLock);

        loader_mutex_unblock_locked(mutex, flags);
    }

    return B_OK;
}

int loader_mutex_switch_lock(int32_t *fromMutex, int32_t *toMutex,
    const char *name, uint32_t flags, int64_t timeout)
{
    if (fromMutex == NULL || ((intptr_t)fromMutex) % 4 != 0)
    {
        return B_BAD_ADDRESS;
    }

    if (toMutex == NULL || ((intptr_t)toMutex) % 4 != 0)
    {
        return B_BAD_ADDRESS;
    }

    int error;

    // unlock the first mutex and lock the second one
    {
        auto lock = std::unique_lock<std::mutex>(sMutexTableLock);
        loader_mutex_unblock_locked(fromMutex, flags);

        error = loader_mutex_lock_locked(toMutex, name, flags, timeout, lock);
    }

    return error;
}

static int loader_mutex_lock_locked(int32_t *mutex, const char *name, uint32_t flags, int64_t timeout,
    std::unique_lock<std::mutex> &lock)
{
    int32_t oldValue = atomic_or(mutex, B_USER_MUTEX_LOCKED | B_USER_MUTEX_WAITING);

    if ((oldValue & (B_USER_MUTEX_LOCKED | B_USER_MUTEX_WAITING)) == 0 || (oldValue & B_USER_MUTEX_DISABLED) != 0)
    {
        // clear the waiting flag and be done
        atomic_and(mutex, ~(int32_t)B_USER_MUTEX_WAITING);
        return B_OK;
    }

    bool lastWaiter;
    int error = loader_mutex_wait_locked(mutex, name, flags, timeout, lock, lastWaiter);

    if (lastWaiter)
    {
        atomic_and(mutex, ~(int32_t)B_USER_MUTEX_WAITING);
    }

    return error;
}

static void loader_mutex_unblock_locked(int32_t *mutex, uint32_t flags)
{
    auto it = sMutexTable.find(mutex);
    if (it == sMutexTable.end() || !it->second || it->second->empty())
    {
        // no one is waiting -- clear locked flag
        atomic_and(mutex, ~(int32_t)B_USER_MUTEX_LOCKED);
        if (it != sMutexTable.end())
        {
            sMutexTable.erase(it);
        }
        return;
    }

    // Someone is waiting -- set the locked flag. It might still be set,
    // but when using userland atomic operations, the caller will usually
    // have cleared it already.
    int32_t oldValue = atomic_or(mutex, B_USER_MUTEX_LOCKED);

    // unblock the first thread
    it->second->front()->locked = true;
    it->second->front()->condition->notify_one();

    if ((flags & B_USER_MUTEX_UNBLOCK_ALL) != 0 || (oldValue & B_USER_MUTEX_DISABLED) != 0)
    {
        it->second->pop_front();

        // unblock and dequeue all the other waiting threads as well
        while (it->second->size())
        {
            it->second->front()->locked = true;
            it->second->front()->condition->notify_one();
            it->second->pop_front();
        }

        // dequeue the first thread and mark the mutex uncontended
        sMutexTable.erase(it);
        atomic_and(mutex, ~(int32_t)B_USER_MUTEX_WAITING);
    }
    else
    {
        bool otherWaiters = remove_loader_mutex_info(mutex, it->second->front());
        if (!otherWaiters)
        {
            atomic_and(mutex, ~(int32_t)B_USER_MUTEX_WAITING);
        }
    }
}

static int loader_mutex_wait_locked(int32_t *mutex, const char *name, uint32_t flags, int64_t timeout,
    std::unique_lock<std::mutex> &lock, bool &lastWaiter)
{
    // add the entry to the table
    std::shared_ptr<mutex_info> entry = std::make_shared<mutex_info>();
    entry->locked = false;

    add_loader_mutex_info(mutex, entry);

    // wait
    int error = loader_condvar_wait(*entry->condition, lock, entry->locked, timeout, flags);

    if (!entry->locked)
    {
        // if nobody woke us up, we have to dequeue ourselves
        lastWaiter = !remove_loader_mutex_info(mutex, entry);
    }
    else
    {
        // otherwise the waker has done the work of marking the
        // mutex or semaphore uncontended
        lastWaiter = false;
    }

    return error;
}

static void add_loader_mutex_info(int32_t *mutex, std::shared_ptr<mutex_info> info)
{
    auto it = sMutexTable.find(mutex);
    if (it == sMutexTable.end())
    {
        it = sMutexTable.emplace(mutex, std::make_shared<std::list<std::shared_ptr<mutex_info>>>()).first;
    }

    info->otherWaiters = it->second;
    it->second->push_back(info);
}

static bool remove_loader_mutex_info(int32_t *mutex, std::shared_ptr<mutex_info> info)
{
    if (info->otherWaiters.expired())
    {
        return false;
    }

    auto list = info->otherWaiters.lock();
    list->remove(info);

    if (list->empty())
    {
        sMutexTable.erase(mutex);
    }

    return list->size();
}

static int loader_condvar_wait(std::condition_variable &cond, std::unique_lock<std::mutex> &lock,
    bool& stopWaiting, int64_t timeout, uint32_t flags)
{
    bool absolute = flags & B_ABSOLUTE_TIMEOUT;
    bool relative = flags & B_RELATIVE_TIMEOUT;

    std::cv_status status = std::cv_status::no_timeout;

    if (timeout == B_INFINITE_TIMEOUT || (!absolute && !relative))
    {
        cond.wait(lock, [&stopWaiting] { return stopWaiting; });
    }
    else if (absolute)
    {
        status = cond.wait_until(lock,
            std::chrono::steady_clock::time_point(std::chrono::microseconds(timeout)));
    }
    else if (relative)
    {
        status = cond.wait_for(lock, std::chrono::microseconds(timeout));
    }

    return status == std::cv_status::no_timeout ? B_OK : B_TIMED_OUT;
}

static int32_t atomic_or(int32_t *a, int32_t b)
{
    static_assert(sizeof(std::atomic<int32_t>) == sizeof(int32_t), "Can't use this hack on this platform.");
    auto atomic_a = (std::atomic<int32_t> *)a;
    return atomic_a->fetch_or(b);
}

static int32_t atomic_and(int32_t *a, int32_t b)
{
    static_assert(sizeof(std::atomic<int32_t>) == sizeof(int32_t), "Can't use this hack on this platform.");
    auto atomic_a = (std::atomic<int32_t> *)a;
    return atomic_a->fetch_and(b);
}