#ifndef __LOADER_MUTEX_H__
#define __LOADER_MUTEX_H__

#include <cstdint>

int loader_mutex_lock(int32_t* mutex, const char* name, uint32_t flags, int64_t timeout);
int loader_mutex_unlock(int32_t* mutex, uint32_t flags);
// Unlocks "from" and locks "to" such that unlocking and starting to wait
// for the lock is atomic. I.e. if "from" guards the object "to" belongs
// to, the operation is safe as long as "from" is held while destroying
// "to".
int loader_mutex_switch_lock(int32_t* fromMutex, int32_t* toMutex,
    const char* name, uint32_t flags, int64_t timeout);

#endif // __LOADER_MUTEX_H__