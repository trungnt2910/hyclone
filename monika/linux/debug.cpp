#include <cstddef>
#include <string.h>

#include "BeDefs.h"
#include "debugbreak.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_debugger.h"
#include "haiku_errors.h"
#include "linux_debug.h"
#include "linux_subsystemlock.h"
#include "linux_syscall.h"
#include "servercalls.h"
#include "thread_defs.h"

static int sDebugSubsystemLock = HYCLONE_SUBSYSTEM_LOCK_UNINITIALIZED;

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
    size_t len = strlen(userString);
    const char prefix[] = "_kern_debugger: ";
    LINUX_SYSCALL3(__NR_write, 2, prefix, sizeof(prefix));
    LINUX_SYSCALL3(__NR_write, 2, userString, len);
    LINUX_SYSCALL3(__NR_write, 2, "\n", 1);

    if (GET_HOSTCALLS()->is_debugger_present())
    {
        debug_break();
    }
    else
    {
        GET_HOSTCALLS()->at_exit(1);

        while (true)
        {
            LINUX_SYSCALL1(__NR_exit_group, 1);
        }
    }
}

port_id MONIKA_EXPORT _kern_install_team_debugger(team_id team, port_id debuggerPort)
{
    return GET_SERVERCALLS()->install_team_debugger(team, debuggerPort);
}

}