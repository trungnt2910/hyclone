#ifndef __HYCLONE_SERVERCALLS_H__
#define __HYCLONE_SERVERCALLS_H__

#include <cstdint>
#include <cstddef>

#define HYCLONE_SERVERCALL0(name) \
    intptr_t (*name)();
#define HYCLONE_SERVERCALL1(name, arg1) \
    intptr_t (*name)(arg1);
#define HYCLONE_SERVERCALL2(name, arg1, arg2) \
    intptr_t (*name)(arg1, arg2);
#define HYCLONE_SERVERCALL3(name, arg1, arg2, arg3) \
    intptr_t (*name)(arg1, arg2, arg3);
#define HYCLONE_SERVERCALL4(name, arg1, arg2, arg3, arg4) \
    intptr_t (*name)(arg1, arg2, arg3, arg4);
#define HYCLONE_SERVERCALL5(name, arg1, arg2, arg3, arg4, arg5) \
    intptr_t (*name)(arg1, arg2, arg3, arg4, arg5);
#define HYCLONE_SERVERCALL6(name, arg1, arg2, arg3, arg4, arg5, arg6) \
    intptr_t (*name)(arg1, arg2, arg3, arg4, arg5, arg6);

struct servercalls
{
#include "servercall_defs.h"
};

#undef HYCLONE_SERVERCALL0
#undef HYCLONE_SERVERCALL1
#undef HYCLONE_SERVERCALL2
#undef HYCLONE_SERVERCALL3
#undef HYCLONE_SERVERCALL4
#undef HYCLONE_SERVERCALL5
#undef HYCLONE_SERVERCALL6

#define HYCLONE_SERVERCALL0(name) \
    SERVERCALL_ID_##name,
#define HYCLONE_SERVERCALL1(name, arg1) \
    SERVERCALL_ID_##name,
#define HYCLONE_SERVERCALL2(name, arg1, arg2) \
    SERVERCALL_ID_##name,
#define HYCLONE_SERVERCALL3(name, arg1, arg2, arg3) \
    SERVERCALL_ID_##name,
#define HYCLONE_SERVERCALL4(name, arg1, arg2, arg3, arg4) \
    SERVERCALL_ID_##name,
#define HYCLONE_SERVERCALL5(name, arg1, arg2, arg3, arg4, arg5) \
    SERVERCALL_ID_##name,
#define HYCLONE_SERVERCALL6(name, arg1, arg2, arg3, arg4, arg5, arg6) \
    SERVERCALL_ID_##name,

enum servercall_id
{
#include "servercall_defs.h"
};

#undef HYCLONE_SERVERCALL0
#undef HYCLONE_SERVERCALL1
#undef HYCLONE_SERVERCALL2
#undef HYCLONE_SERVERCALL3
#undef HYCLONE_SERVERCALL4
#undef HYCLONE_SERVERCALL5
#undef HYCLONE_SERVERCALL6

#define HYCLONE_SOCKET_NAME ".hyclone.sock"
#define HYCLONE_SHM_NAME ".hyclone.shm"
#define HYCLONE_SERVERCALL_MAX_ARGS (6)

#endif // __HYCLONE_SERVERCALLS_H__