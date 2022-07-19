#include <deque>
#include <mutex>

#include "extended_commpage.h"
#include "loader_lock.h"

static std::mutex sMasterLock;
static std::deque<std::mutex> sSubsystemLocks;

void loader_lock_subsystem(int* subsystemId)
{
    if (*subsystemId != HYCLONE_SUBSYSTEM_LOCK_UNINITIALIZED)
    {
        sSubsystemLocks[*subsystemId].lock();
        return;
    }

    auto masterLock = std::unique_lock(sMasterLock);
    sSubsystemLocks.resize(sSubsystemLocks.size() + 1);
    *subsystemId = (int)sSubsystemLocks.size() - 1;
    sSubsystemLocks[*subsystemId].lock();
}

void loader_unlock_subsystem(int subsystemId)
{
    sSubsystemLocks[subsystemId].unlock();
}
