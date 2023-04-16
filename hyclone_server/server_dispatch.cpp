#include <cstddef>
#include "haiku_errors.h"
#include "process.h"
#include "servercalls.h"
#include "server_servercalls.h"
#include "system.h"
#include "thread.h"

intptr_t server_dispatch(intptr_t conn_id, intptr_t call_id,
    intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6)
{
    hserver_context context;
    context.conn_id = conn_id;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        switch (call_id)
        {
            case SERVERCALL_ID_connect:
            case SERVERCALL_ID_request_ack:
                std::tie(context.pid, context.tid) = std::tie(a1, a2);
            break;
            default:
                const auto [pid, tid, isPrimary] = system.GetThreadFromConnection(conn_id);
                std::tie(context.pid, context.tid) = std::tie(pid, tid);
            break;
        }

        // For SERVERCALL_ID_connect, the process and thread hasn't been registered
        // in HyClone server yet.
        if (call_id != SERVERCALL_ID_connect)
        {
            context.process = system.GetProcess(context.pid).lock();
            if (!context.process)
            {
                return B_BAD_VALUE;
            }
        }
    }

    if (context.process)
    {
        auto procLock = context.process->Lock();
        context.thread = context.process->GetThread(context.tid).lock();

        if (!context.thread)
        {
            return B_BAD_VALUE;
        }
    }

    intptr_t result = HAIKU_POSIX_ENOSYS;

#define HYCLONE_SERVERCALL0(name) \
    case SERVERCALL_ID_##name: \
        result = server_hserver_call_##name(context); break;
#define HYCLONE_SERVERCALL1(name, arg1) \
    case SERVERCALL_ID_##name: \
        result = server_hserver_call_##name(context, (arg1)a1); break;
#define HYCLONE_SERVERCALL2(name, arg1, arg2) \
    case SERVERCALL_ID_##name: \
        result = server_hserver_call_##name(context, (arg1)a1, (arg2)a2); break;
#define HYCLONE_SERVERCALL3(name, arg1, arg2, arg3) \
    case SERVERCALL_ID_##name: \
        result = server_hserver_call_##name(context, (arg1)a1, (arg2)a2, (arg3)a3); break;
#define HYCLONE_SERVERCALL4(name, arg1, arg2, arg3, arg4) \
    case SERVERCALL_ID_##name: \
        result = server_hserver_call_##name(context, (arg1)a1, (arg2)a2, (arg3)a3, (arg4)a4); break;
#define HYCLONE_SERVERCALL5(name, arg1, arg2, arg3, arg4, arg5) \
    case SERVERCALL_ID_##name: \
        result = server_hserver_call_##name(context, (arg1)a1, (arg2)a2, (arg3)a3, (arg4)a4, (arg5)a5); break;
#define HYCLONE_SERVERCALL6(name, arg1, arg2, arg3, arg4, arg5, arg6) \
    case SERVERCALL_ID_##name: \
        result = server_hserver_call_##name(context, (arg1)a1, (arg2)a2, (arg3)a3, (arg4)a4, (arg5)a5, (arg6)a6); break;

    switch (call_id)
    {
#include "servercall_defs.h"
    }

#undef HYCLONE_SERVERCALL0
#undef HYCLONE_SERVERCALL1
#undef HYCLONE_SERVERCALL2
#undef HYCLONE_SERVERCALL3
#undef HYCLONE_SERVERCALL4
#undef HYCLONE_SERVERCALL5
#undef HYCLONE_SERVERCALL6

    return result;
}