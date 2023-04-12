#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <unordered_map>

#include "BeDefs.h"
#include "haiku_debugger.h"
#include "haiku_errors.h"
#include "haiku_thread.h"
#include "loader_debugger.h"
#include "loader_ids.h"
#include "loader_servercalls.h"
#include "loader_spawn_thread.h"
#include "loader_sysinfo.h"
#include "loader_systemtime.h"
#include "thread_defs.h"

static std::atomic<bool> sDebuggerInstalling = false;
static std::atomic<bool> sDebuggerInstalled = false;
static port_id sDebuggerPort = -1;
static sem_id sDebuggerWriteLock = -1;
static port_id sNubPort = -1;
static thread_id sNubThread = -1;
static void* sNubStack = NULL;
static std::atomic<int> sTeamFlags = 0;
std::mutex sThreadFlagsMutex;
std::unordered_map<int, int> sThreadFlags;
static thread_local bigtime_t sSyscallStart = -1;

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
    if (sDebuggerInstalling.exchange(true))
    {
        return B_NOT_ALLOWED;
    }

    if (sDebuggerInstalled)
    {
        sDebuggerInstalling = false;
        return B_NOT_ALLOWED;
    }

    int pid = loader_get_pid();

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

    sTeamFlags = 0;
    sThreadFlags.clear();
    sDebuggerPort = debuggerPort;

    // Everything went fine, resume the nub thread.
    loader_hserver_call_resume_thread(sNubThread);

    sDebuggerInstalled = true;
    sDebuggerInstalling = false;
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

    sDebuggerInstalled = false;
    sDebuggerInstalling = false;
    return error;
}

static void loader_write_debug_message(int code, debug_debugger_message_data& message)
{
    int tid = loader_get_tid();

    message.origin.nub_port = sNubPort;
    message.origin.thread = tid;
    message.origin.team = loader_get_pid();

    if (loader_hserver_call_acquire_sem(sDebuggerWriteLock) != B_OK)
    {
        // Probably the debugger is gone.
        return;
    }

    // Does not actually suspend the thread, just set some internal flags.
    loader_hserver_call_suspend_thread(tid);

    loader_hserver_call_write_port_etc(sDebuggerPort, code, &message, sizeof(message), 0, B_INFINITE_TIMEOUT);
    loader_hserver_call_release_sem(sDebuggerWriteLock);

    // The thread is actually suspended.
    loader_hserver_call_wait_for_resume();
}

void loader_debugger_pre_syscall(uint32_t callIndex, void* args)
{
    if (sDebuggerInstalled)
    {
        bool debugPreSyscall = (sTeamFlags & B_TEAM_DEBUG_PRE_SYSCALL);
        bool debugPostSyscall = (sTeamFlags & B_TEAM_DEBUG_POST_SYSCALL);

        if (!debugPreSyscall || !debugPostSyscall)
        {
            auto lock = std::unique_lock<std::mutex>(sThreadFlagsMutex);
            int threadFlags = sThreadFlags[loader_get_tid()];
            debugPreSyscall = debugPreSyscall || (threadFlags & B_THREAD_DEBUG_PRE_SYSCALL);
            debugPostSyscall = debugPostSyscall || (threadFlags & B_THREAD_DEBUG_POST_SYSCALL);
        }

        if (debugPostSyscall)
        {
            sSyscallStart = loader_system_time();
        }

        if (debugPreSyscall)
        {
            debug_debugger_message_data message;

            memcpy(message.pre_syscall.args, args, sizeof(message.pre_syscall.args));
            message.pre_syscall.syscall = callIndex;

            loader_write_debug_message(B_DEBUGGER_MESSAGE_PRE_SYSCALL, message);
        }
    }
}

void loader_debugger_post_syscall(uint32_t callIndex, void* args, uint64_t returnValue)
{
    if (sDebuggerInstalled)
    {
        int tid = loader_get_tid();

        bool debugPostSyscall = (sTeamFlags & B_TEAM_DEBUG_POST_SYSCALL);

        if (!debugPostSyscall)
        {
            auto lock = std::unique_lock<std::mutex>(sThreadFlagsMutex);
            debugPostSyscall = (sThreadFlags[tid] & B_THREAD_DEBUG_POST_SYSCALL);
        }

        if (debugPostSyscall && sSyscallStart >= 0)
        {
            debug_debugger_message_data message;

            memcpy(message.post_syscall.args, args, sizeof(message.post_syscall.args));
            message.post_syscall.syscall = callIndex;
            message.post_syscall.return_value = returnValue;
            message.post_syscall.start_time = sSyscallStart;
            message.post_syscall.end_time = loader_system_time();

            sSyscallStart = -1;

            loader_write_debug_message(B_DEBUGGER_MESSAGE_POST_SYSCALL, message);
        }
    }
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
            case B_DEBUG_MESSAGE_SET_TEAM_FLAGS:
                sTeamFlags = data.set_team_flags.flags;
            break;
            case B_DEBUG_MESSAGE_SET_THREAD_FLAGS:
            {
                auto lock = std::unique_lock<std::mutex>(sThreadFlagsMutex);
                sThreadFlags[data.set_thread_flags.thread] = data.set_thread_flags.flags;
            }
            break;
            case B_DEBUG_MESSAGE_CONTINUE_THREAD:
            {
                loader_hserver_call_resume_thread(data.continue_thread.thread);
            }
            break;
            default:
            {
                std::cerr << "Unimplemented nub thread operation: " << op << std::endl;
            }
        }
    }
}