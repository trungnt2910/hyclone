#include <cstddef>

#include "BeDefs.h"
#include "extended_commpage.h"
#include "haiku_tls.h"
#include "runtime_loader.h"

// Both are present in both libroot and runtime_loader.
extern rld_export *__gRuntimeLoader;
extern void *__gCommPageAddress;

extern "C" thread_id _kern_find_thread(const char* name);

extern "C" void _kern_debug_output(const char* name);

struct tls_index
{
    unsigned long int module;
    unsigned long int offset;
};

extern "C"
{

void *
__tls_get_addr(tls_index *ti)
{
    return __gRuntimeLoader->get_tls_address(ti->module, ti->offset);
}

static void* buffer[TLS_MAX_KEYS] = { NULL };

int32
tls_allocate()
{
    if (__gCommPageAddress != NULL)
        return GET_HOSTCALLS()->tls_allocate();

    return -1;
}

void *
tls_get(int32 index)
{
    if (__gCommPageAddress != NULL)
        return GET_HOSTCALLS()->tls_get(index);

    return buffer[index];
}

void **
tls_address(int32 index)
{
    if (__gCommPageAddress != NULL)
        return GET_HOSTCALLS()->tls_address(index);

    return &buffer[index];
}

void
tls_set(int32 index, void *value)
{
    if (__gCommPageAddress != NULL)
        GET_HOSTCALLS()->tls_set(index, value);
    else
        buffer[index] = value;
}

thread_id
find_thread(const char *name)
{
    if (name == NULL)
    {
        return (thread_id)(intptr_t)tls_get(TLS_THREAD_ID_SLOT);
    }

    return _kern_find_thread(name);
}
}