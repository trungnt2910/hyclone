#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>

#ifdef HYCLONE_HOST_LINUX
#include <unistd.h>
#else
#error Include the header for getpid
#endif

#include "BeDefs.h"
#include "haiku_debugger.h"
#include "haiku_errors.h"
#include "haiku_thread.h"
#include "loader_debugger.h"
#include "loader_servercalls.h"
#include "loader_sysinfo.h"
#include "loader_spawn_thread.h"
#include "thread_defs.h"

static std::atomic<bool> sDebuggerInstalled = false;
static port_id sDebuggerPort = -1;
static sem_id sDebuggerWriteLock = -1;
static port_id sNubPort = -1;
static thread_id sNubThread = -1;
static void* sNubStack = NULL;

static int loader_debugger_nub_thread_entry(void*, void*);

int loader_dprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vfprintf(stderr, format, args);
    va_end(args);

    return result;
}

int loader_install_team_debugger(int debuggerTeam, int debuggerPort)
{
    // TODO: Implement Haiku's handover mechanism.
    if (sDebuggerInstalled.exchange(true))
    {
        return B_NOT_ALLOWED;
    }

    int pid = getpid();

    char nameBuffer[B_OS_NAME_LENGTH];
    size_t nameLen;
    status_t error = B_OK;

    // create the debugger write lock semaphore
    nameLen = snprintf(nameBuffer, sizeof(nameBuffer), "team %d debugger port write", pid);
    sDebuggerWriteLock = loader_hserver_call_create_sem(1, nameBuffer, nameLen);
    if (sDebuggerWriteLock < 0)
    {
        error = sDebuggerWriteLock;
        sDebuggerWriteLock = -1;
        goto clean_and_die;
    }

    // create the nub port
    nameLen = snprintf(nameBuffer, sizeof(nameBuffer), "team %d debug", pid);
    sNubPort = loader_hserver_call_create_port(1, nameBuffer, nameLen);
    if (sNubPort < 0)
    {
        error = sNubPort;
        sNubPort = -1;
        goto clean_and_die;
    }

    // make the debugger team the port owner; thus we know, if the debugger is
    // gone and can cleanup
    error = loader_hserver_call_set_port_owner(sNubPort, debuggerTeam);
    if (error != B_OK)
    {
        goto clean_and_die;
        return error;
    }

    // spawn the nub thread
    snprintf(nameBuffer, sizeof(nameBuffer), "team %d debug task", pid);

    if (sNubStack == NULL)
    {
        sNubStack = aligned_alloc(B_PAGE_SIZE, USER_STACK_SIZE + USER_STACK_GUARD_SIZE);
    }

    thread_creation_attributes threadInfo;
    memset(&threadInfo, 0, sizeof(threadInfo));
    threadInfo.entry = loader_debugger_nub_thread_entry;
    threadInfo.name = nameBuffer;
    threadInfo.priority = B_NORMAL_PRIORITY;
    threadInfo.args1 = NULL;
    threadInfo.args2 = NULL;
    threadInfo.guard_size = USER_STACK_GUARD_SIZE;
    threadInfo.stack_size = USER_STACK_SIZE;
    threadInfo.stack_address = (void*)((uintptr_t)sNubStack + USER_STACK_GUARD_SIZE);
    // Should be safe as no Haiku code is called in the thread.
    threadInfo.pthread = NULL;
    threadInfo.flags = 0;

    sNubThread = loader_spawn_thread(&threadInfo);
    if (sNubThread < 0)
    {
        error = sNubThread;
        sNubThread = -1;
        goto clean_and_die;
    }

    loader_hserver_call_register_nub(sNubPort, sNubThread, sDebuggerWriteLock);

    // Everything went fine, resume the nub thread.
    loader_hserver_call_resume_thread(sNubThread);

    return sNubPort;
clean_and_die:
    if (error != B_OK)
    {
        if (sNubPort >= 0)
        {
            loader_hserver_call_delete_port(sNubPort);
            sNubPort = -1;
        }

        if (sDebuggerWriteLock >= 0)
        {
            loader_hserver_call_delete_sem(sDebuggerWriteLock);
            sDebuggerWriteLock = -1;
        }

        sDebuggerInstalled = false;
    }

    return error;
}

int loader_remove_team_debugger()
{
    // Delete the nub port -- this will cause the nub thread to terminate and
    // remove the debugger.
    if (sNubPort >= 0)
    {
        loader_hserver_call_delete_port(sNubPort);
    }

    if (sNubThread >= 0)
    {
        loader_wait_for_thread(sNubThread, NULL);
    }

    sDebuggerInstalled = false;
    return B_OK;
}

static int loader_debugger_nub_thread_entry(void*, void*)
{
    debug_nub_message_data data;
    int32 op;

    while (true)
    {
        // sNubPort shoud never change while this thread is alive.
        status_t error = loader_hserver_call_read_port_etc(sNubPort, &op, &data, sizeof(data),
            0, B_INFINITE_TIMEOUT);

        if (error != B_OK)
        {
            // The debugger port is closed.
            // This probably means the debugger is dead.
            sDebuggerInstalled = false;
            return 0;
        }

        switch (op)
        {
            default:
            {
                std::cerr << "Unimplemented nub thread operation: " << op << std::endl;
            }
        }
    }
}