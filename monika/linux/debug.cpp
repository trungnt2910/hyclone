#include <cstddef>
#include <string.h>

#include "BeDefs.h"
#include "debugbreak.h"
#include "extended_commpage.h"
#include "haiku_debugger.h"
#include "haiku_errors.h"
#include "linux_debug.h"
#include "linux_subsystemlock.h"
#include "linux_syscall.h"
#include "servercalls.h"
#include "thread_defs.h"

extern "C"
{

void _moni_debug_output(const char* userString)
{
    GET_SERVERCALLS()->debug_output(userString, strlen(userString));
}

void _moni_debugger(const char* userString)
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

void _moni_register_syslog_daemon(port_id port)
{
    _moni_debug_output("stub: _kern_register_syslog_daemon");
}

status_t _moni_install_default_debugger(port_id debuggerPort)
{
    _moni_debug_output("stub: _kern_start_watching");
    return HAIKU_POSIX_ENOSYS;
}

port_id _moni_install_team_debugger(team_id team, port_id debuggerPort)
{
    return GET_SERVERCALLS()->install_team_debugger(team, debuggerPort);
}

}