#ifndef __LINUX_SUBSYSTEMLOCK_H__
#define __LINUX_SUBSYSTEMLOCK_H__

#include "extended_commpage.h"

class SubsystemLock
{
private:
    int* _id;
public:
    SubsystemLock(int& subsystemId)
        : _id(&subsystemId)
    {
        GET_HOSTCALLS()->lock_subsystem(_id);
    }
    ~SubsystemLock()
    {
        GET_HOSTCALLS()->unlock_subsystem(*_id);
    }
};

#endif