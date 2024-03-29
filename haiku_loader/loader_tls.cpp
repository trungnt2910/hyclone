#include <atomic>
#include <cstdint>
#include <cstdio>

#include "haiku_errors.h"
#include "haiku_loader.h"
#include "haiku_tls.h"
#include "loader_ids.h"
#include "loader_servercalls.h"
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

void loader_init_tls()
{
    sProcessTls[TLS_BASE_ADDRESS_SLOT] = &sProcessTls;
    sProcessTls[TLS_THREAD_ID_SLOT] = (void*)(intptr_t)loader_get_tid();
    sProcessTls[TLS_USER_THREAD_SLOT] = &sUserThread;
    sProcessTls[TLS_DYNAMIC_THREAD_VECTOR] = NULL;
    sUserThread.pthread = NULL;
    sUserThread.flags = 0;
    sUserThread.defer_signals = 0;
    sUserThread.wait_status = 0;
    sUserThread.pending_signals = 0;
    loader_hserver_call_register_user_thread(&sUserThread);
}