#include <cstddef>
#include <errno.h>
#include <unistd.h>

#include "haiku_tls.h"
#include "loader_fork.h"
#include "loader_servercalls.h"
#include "loader_tls.h"

extern bool loader_hserver_child_atfork();

// This function handles the forking of process states
// (runtime_loader, commpage, kernel connections,...)
int loader_fork()
{
    // Kernel connections are handled by pthread_atfork.
    int status = fork();
    if (status < 0)
    {
        return -errno;
    }
    if (status == 0)
    {
        // Set the thread_id slot.
        // libroot's getpid() function depends on it.
        tls_set(TLS_THREAD_ID_SLOT, (void*)(uintptr_t)gettid());
        loader_hserver_child_atfork();
    }
    return status;
}