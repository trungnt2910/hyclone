#ifndef __LOADER_SERVERCALLS_H__
#define __LOADER_SERVERCALLS_H__

bool loader_init_servercalls();

#define HYCLONE_SERVERCALL0(name) \
    intptr_t loader_hserver_call_##name();
#define HYCLONE_SERVERCALL1(name, arg1) \
    intptr_t loader_hserver_call_##name(arg1 a1);
#define HYCLONE_SERVERCALL2(name, arg1, arg2) \
    intptr_t loader_hserver_call_##name(arg1 a1, arg2 a2);
#define HYCLONE_SERVERCALL3(name, arg1, arg2, arg3) \
    intptr_t loader_hserver_call_##name(arg1 a1, arg2 a2, arg3 a3);
#define HYCLONE_SERVERCALL4(name, arg1, arg2, arg3, arg4) \
    intptr_t loader_hserver_call_##name(arg1 a1, arg2 a2, arg3 a3, arg4 a4);
#define HYCLONE_SERVERCALL5(name, arg1, arg2, arg3, arg4, arg5) \
    intptr_t loader_hserver_call_##name(arg1 a1, arg2 a2, arg3 a3, arg4 a4, arg5 a5);
#define HYCLONE_SERVERCALL6(name, arg1, arg2, arg3, arg4, arg5, arg6) \
    intptr_t loader_hserver_call_##name(arg1 a1, arg2 a2, arg3 a3, arg4 a4, arg5 a5, arg6 a6);

#include "servercall_defs.h"

#undef HYCLONE_SERVERCALL0
#undef HYCLONE_SERVERCALL1
#undef HYCLONE_SERVERCALL2
#undef HYCLONE_SERVERCALL3
#undef HYCLONE_SERVERCALL4
#undef HYCLONE_SERVERCALL5
#undef HYCLONE_SERVERCALL6


#endif // __LOADER_SERVERCALLS_H__