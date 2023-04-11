#include <iostream>
#include <string>

#include "haiku_errors.h"
#include "port.h"
#include "process.h"
#include "server_requests.h"
#include "server_servercalls.h"
#include "server_workers.h"
#include "system.h"
#include "thread.h"

intptr_t server_hserver_call_debug_output(hserver_context& context, const char *userString, size_t len)
{
    std::string message(len, '\0');

    if (context.process->ReadMemory((void*)userString, message.data(), len) != len)
    {
        return B_BAD_ADDRESS;
    }

    auto& system = System::GetInstance();
    auto lock = system.Lock();

    // TODO: Make a dedicated log function
    std::cerr << "[" << context.pid << "] [" << context.tid << "] _kern_debug_output: " << message << std::endl;

    return B_OK;
}

intptr_t server_hserver_call_install_team_debugger(hserver_context& context, int team, int port)
{
    std::shared_ptr<Process> targetProcess;
    std::shared_ptr<Thread> targetThread;
    std::shared_ptr<Port> debuggerPort;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        targetProcess = system.GetProcess(team).lock();
        if (!targetProcess)
        {
            return B_BAD_TEAM_ID;
        }

        targetThread = system.GetThread(team).lock();
        if (!targetThread)
        {
            return B_BAD_TEAM_ID;
        }

        debuggerPort = system.GetPort(port).lock();
        if (!debuggerPort)
        {
            return B_BAD_PORT_ID;
        }
    }

    {
        auto lock = targetProcess->Lock();

        // TODO: Haiku has a debugger "handover" mechanism that
        // allows a debugger to take over a debuggee from another
        // debugger.
        if (targetProcess->GetDebuggerPid() != -1)
        {
            return B_NOT_ALLOWED;
        }
    }

    status_t status;

    {
        auto requestLock = server_worker_run_wait([&]()
        {
            return targetThread->RequestLock();
        });

        std::shared_future<intptr_t> result;

        {
            auto lock = targetThread->Lock();

            result = targetThread->SendRequest(
                std::make_shared<InstallTeamDebuggerRequest>(
                    InstallTeamDebuggerRequestArgs{
                        REQUEST_ID_install_team_debugger,
                        context.pid,
                        debuggerPort->GetId() }));
        }

        status = server_worker_run_wait([&]()
        {
            return result.get();
        });
    }

    if (status < 0)
    {
        return status;
    }

    {
        auto lock = targetProcess->Lock();

        targetProcess->SetDebuggerPid(context.pid);
        targetProcess->SetDebuggerPort(port);
    }

    return status;
}

intptr_t server_hserver_call_register_nub(hserver_context& context, int port, int thread, int writeLock)
{
    int debuggerPid;
    std::shared_ptr<Process> debuggerProcess;

    {
        auto lock = context.process->Lock();

        context.process->SetDebuggerWriteLock(writeLock);
        context.process->GetInfo().debugger_nub_port = port;
        context.process->GetInfo().debugger_nub_thread = thread;

        context.process->RemoveOwningSemaphore(writeLock);

        debuggerPid = context.process->GetDebuggerPid();
    }

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        debuggerProcess = system.GetProcess(debuggerPid).lock();
    }

    if (debuggerProcess)
    {
        debuggerProcess->AddOwningSemaphore(writeLock);
    }

    return B_OK;
}