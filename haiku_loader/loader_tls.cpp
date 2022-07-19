#include <atomic>
#include <cstdint>
#include <cstdio>

#ifdef __linux__
#include <unistd.h> // syscall(SYS_gettid)
#include <sys/syscall.h>
#else
#include <thread>   // std::this_thread
#endif

#include "haiku_errors.h"
#include "haiku_loader.h"
#include "haiku_tls.h"
#include "user_thread_defs.h"

#include "loader_tls.h"

// TLS support is implemented here in haiku_loader
// to take advantage of the full C++ library on the host
// system, as well as the ability to use the host's
// thread_local storage.
static thread_local void* sProcessTls[TLS_MAX_KEYS];
static thread_local user_thread sUserThread;
static std::atomic<int> sNextSlot(TLS_FIRST_FREE_SLOT);

void** get_tls()
{
    return sProcessTls;
}

int32_t tls_allocate()
{
    if (sNextSlot < TLS_MAX_KEYS) {
        auto next = sNextSlot++;
        if (next < TLS_MAX_KEYS)
            return next;
    }

    return B_NO_MEMORY;
}


void* tls_get(int32_t index)
{
    return get_tls()[index];
}


void** tls_address(int32_t index)
{
    return get_tls() + index;
}


void tls_set(int32_t index, void* value)
{
    get_tls()[index] = value;
}

static intptr_t loader_get_thread_id()
{
#ifdef __linux__
    return (intptr_t)syscall(SYS_gettid);
#else
    return (intptr_t)std::hash<std::thread::id>()(std::this_thread::get_id());
#endif
}

void loader_init_tls()
{
    sProcessTls[TLS_BASE_ADDRESS_SLOT] = &sProcessTls;
    sProcessTls[TLS_THREAD_ID_SLOT] = (void*)loader_get_thread_id();
    sProcessTls[TLS_USER_THREAD_SLOT] = &sUserThread;
    sProcessTls[TLS_DYNAMIC_THREAD_VECTOR] = NULL;
    sUserThread.pthread = NULL;
    sUserThread.flags = 0;
    sUserThread.defer_signals = 0;
    sUserThread.wait_status = 0;
    sUserThread.pending_signals = 0;
}