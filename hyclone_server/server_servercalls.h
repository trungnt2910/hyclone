#ifndef __SERVER_SERVERCALLS_H__
#define __SERVER_SERVERCALLS_H__

#include <cstddef>
#include <memory>

class Process;
class Thread;

struct hserver_context
{
    intptr_t conn_id;
    int pid;
    int tid;

    std::shared_ptr<Process> process;
    std::shared_ptr<Thread> thread;
};

#define HYCLONE_SERVERCALL0(name) \
    intptr_t server_hserver_call_##name(hserver_context& context);
#define HYCLONE_SERVERCALL1(name, arg1) \
    intptr_t server_hserver_call_##name(hserver_context& context, arg1 a1);
#define HYCLONE_SERVERCALL2(name, arg1, arg2) \
    intptr_t server_hserver_call_##name(hserver_context& context, arg1 a1, arg2 a2);
#define HYCLONE_SERVERCALL3(name, arg1, arg2, arg3) \
    intptr_t server_hserver_call_##name(hserver_context& context, arg1 a1, arg2 a2, arg3 a3);
#define HYCLONE_SERVERCALL4(name, arg1, arg2, arg3, arg4) \
    intptr_t server_hserver_call_##name(hserver_context& context, arg1 a1, arg2 a2, arg3 a3, arg4 a4);
#define HYCLONE_SERVERCALL5(name, arg1, arg2, arg3, arg4, arg5) \
    intptr_t server_hserver_call_##name(hserver_context& context, arg1 a1, arg2 a2, arg3 a3, arg4 a4, arg5 a5);
#define HYCLONE_SERVERCALL6(name, arg1, arg2, arg3, arg4, arg5, arg6) \
    intptr_t server_hserver_call_##name(hserver_context& context, arg1 a1, arg2 a2, arg3 a3, arg4 a4, arg5 a5, arg6 a6);

#include "servercall_defs.h"

#undef HYCLONE_SERVERCALL0
#undef HYCLONE_SERVERCALL1
#undef HYCLONE_SERVERCALL2
#undef HYCLONE_SERVERCALL3
#undef HYCLONE_SERVERCALL4
#undef HYCLONE_SERVERCALL5
#undef HYCLONE_SERVERCALL6

intptr_t server_dispatch(intptr_t conn_id,
    intptr_t call_id, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6);

extern thread_local hserver_context* gCurrentContext;

#endif // __SERVER_SERVERCALLS_H__