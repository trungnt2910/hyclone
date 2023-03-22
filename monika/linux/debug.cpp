#include <cstddef>
#include <string.h>
#include <signal.h>

#include "BeDefs.h"
#include "debugbreak.h"
#include "export.h"
#include "haiku_debugger.h"
#include "haiku_errors.h"
#include "linux_debug.h"
#include "linux_subsystemlock.h"
#include "linux_syscall.h"
#include "servercalls.h"
#include "thread_defs.h"

static bool sIsProcessDebugged = false;
static int sDebugSubsystemLock = HYCLONE_SUBSYSTEM_LOCK_UNINITIALIZED;

static int32 NubThreadEntry(void* arg1, void* arg2);

extern "C"
{

extern thread_id _kern_spawn_thread(struct thread_creation_attributes* attributes);
extern status_t _kern_resume_thread(thread_id thread);
extern size_t _kern_read_port_etc(port_id port, int32 *msgCode,
    void *msgBuffer, size_t bufferSize, uint32 flags,
    bigtime_t timeout);

void MONIKA_EXPORT _kern_debug_output(const char* userString)
{
    GET_SERVERCALLS()->debug_output(userString, strlen(userString));
}

void MONIKA_EXPORT _kern_debugger(const char* userString)
{
    if (!sIsProcessDebugged)
    {
        size_t len = strlen(userString);
        const char prefix[] = "_kern_debugger: ";
        LINUX_SYSCALL3(__NR_write, 2, prefix, sizeof(prefix));
        LINUX_SYSCALL3(__NR_write, 2, userString, len);
        LINUX_SYSCALL3(__NR_write, 2, "\n", 1);
    }

    debug_break();
}

status_t MONIKA_EXPORT __monika_spawn_nub_thread(port_id id)
{
    {
        auto lock = SubsystemLock(sDebugSubsystemLock);
        if (sIsProcessDebugged)
        {
            return B_NOT_ALLOWED;
        }

        sIsProcessDebugged = true;
    }
    struct thread_creation_attributes attributes;
    memset(&attributes, 0, sizeof(attributes));

    attributes.args1 = (void*)(intptr_t)id;
    attributes.name = "team debug task";
    attributes.entry = NubThreadEntry;

    thread_id thread_id = _kern_spawn_thread(&attributes);
    _kern_resume_thread(id);

    int tid = LINUX_SYSCALL0(__NR_gettid);

    LINUX_SYSCALL3(__NR_tgkill, -1, tid, SIGINT);

    return B_OK;
}

}

// Forwards everything back to the server.
// On Haiku, the nub thread should be an in-process
// thread that handles these operation, the fact that
// this function forwards the operations to the kernel server
// is Hyclone's implementation detail.
int NubThreadEntry(void* arg1, void* arg2)
{
    port_id port = (port_id)(intptr_t)arg1;

    while (sIsProcessDebugged)
    {
        int command;
        debug_nub_message_data data;

        if (_kern_read_port_etc(port, &command, &data, sizeof(data), 0, 0) < 0)
        {
            continue;
        }

        panic("NubThreadEntry: unimplemented");
        // GET_SERVERCALLS()->debug_process_nub_message(command, &data);
    }
}