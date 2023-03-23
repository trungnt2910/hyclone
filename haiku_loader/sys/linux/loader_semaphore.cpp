#include <cstdlib>
#include <mutex>
#include <semaphore.h>
#include <unordered_map>

#include "haiku_errors.h"
#include "loader_semaphore.h"

std::mutex gSemaphoresMutex;
std::unordered_map<sem_t *, haiku_sem_t*> gSemaphores;

int loader_realtime_sem_open(const char *name, int openFlagsOrShared, haiku_mode_t mode,
    uint32_t semCount, haiku_sem_t* sem, haiku_sem_t** usedSem)
{
    sem_t *linuxSem = sem_open(name, openFlagsOrShared, mode, semCount);
    if (linuxSem == SEM_FAILED)
    {
        return -errno;
    }

    auto lock = std::unique_lock<std::mutex>(gSemaphoresMutex);

    if (!gSemaphores.contains(linuxSem))
    {
        gSemaphores[linuxSem] = sem;
        *usedSem = sem;
    }
    else
    {
        *usedSem = gSemaphores[linuxSem];
    }

    return 0;
}
