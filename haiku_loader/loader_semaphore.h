#ifndef __LOADER_SEMAPHORE_H__
#define __LOADER_SEMAPHORE_H__

#include "BeDefs.h"
#include "haiku_semaphore.h"

int loader_realtime_sem_open(const char *name, int openFlagsOrShared, haiku_mode_t mode,
    uint32_t semCount, haiku_sem_t* sem, haiku_sem_t** usedSem);

#endif // __LOADER_SEMAPHORE_H__
