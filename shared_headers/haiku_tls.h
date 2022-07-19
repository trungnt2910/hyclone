#ifndef __HAIKU_KERNEL_TLS_H__
#define __HAIKU_KERNEL_TLS_H__

/* A maximum of 64 keys is allowed to store in TLS - the key is reserved
 * process-wide. Note that tls_allocate() will return B_NO_MEMORY if you
 * try to exceed this limit.
 */
#define TLS_MAX_KEYS 64

#define TLS_SIZE (TLS_MAX_KEYS * sizeof(void *))
#define TLS_COMPAT_SIZE (TLS_MAX_KEYS * sizeof(uint32))

enum
{
    TLS_BASE_ADDRESS_SLOT = 0,
    // contains the address of the local storage space

    TLS_THREAD_ID_SLOT,
    TLS_ERRNO_SLOT,
    TLS_ON_EXIT_THREAD_SLOT,
    TLS_USER_THREAD_SLOT,
    TLS_DYNAMIC_THREAD_VECTOR,

    // Note: these entries can safely be changed between
    // releases; 3rd party code always calls tls_allocate()
    // to get a free slot

    TLS_FIRST_FREE_SLOT
    // the first free slot for user allocations
};

void** get_tls();

#endif /* __HAIKU_KERNEL_TLS_H__ */