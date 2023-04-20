#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

#include "BeDefs.h"
#include "haiku_area.h"
#include "haiku_debugger.h"
#include "haiku_errors.h"
#include "haiku_thread.h"
#include "loader_debugger.h"
#include "loader_ids.h"
#include "loader_runtime.h"
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

void loader_debugger_reset()
{
    sDebuggerInstalled = false;
    sDebuggerInstalling = false;
    sDebuggerPort = -1;
    sDebuggerWriteLock = -1;
    sNubPort = -1;
    sNubThread = -1;
    sTeamFlags = 0;
    sSyscallStart = -1;
    auto lock = std::unique_lock<std::mutex>(sThreadFlagsMutex);
    sThreadFlags.clear();
}

std::string loader_debugger_serialize_info()
{
    std::stringstream ss;
    ss << sDebuggerInstalled << ","
       << sDebuggerPort << ","
       << sDebuggerWriteLock << ","
       << sNubPort << ","
       << sTeamFlags << ","
       << sThreadFlags[loader_get_tid()];

    return ss.str();
}

void loader_debugger_restore_info(const std::string& s)
{
    if (s.empty())
    {
        return;
    }

    std::stringstream ss(s);

    std::string token;
    std::getline(ss, token, ',');
    sDebuggerInstalled = std::stoi(token);
    std::getline(ss, token, ',');
    sDebuggerPort = std::stoi(token);
    std::getline(ss, token, ',');
    sDebuggerWriteLock = std::stoi(token);
    std::getline(ss, token, ',');
    sNubPort = std::stoi(token);
    std::getline(ss, token, ',');
    sTeamFlags = std::stoi(token);
    std::getline(ss, token, ',');
    sThreadFlags[loader_get_tid()] = std::stoi(token);

    if (sDebuggerInstalled)
    {
        char nameBuffer[B_OS_NAME_LENGTH];
        // spawn the nub thread
        snprintf(nameBuffer, sizeof(nameBuffer), "team %d debug task", loader_get_pid());

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
        loader_hserver_call_resume_thread(sNubThread);
    }
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

        if (debugPreSyscall)
        {
            debug_debugger_message_data message;

            memcpy(message.pre_syscall.args, args, sizeof(message.pre_syscall.args));
            message.pre_syscall.syscall = callIndex;

            loader_write_debug_message(B_DEBUGGER_MESSAGE_PRE_SYSCALL, message);
        }

        if (debugPostSyscall)
        {
            sSyscallStart = loader_system_time();
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

        if (debugPostSyscall && sSyscallStart != (bigtime_t)-1)
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

void loader_debugger_thread_created(thread_id newThread)
{
    if (sDebuggerInstalled)
    {
        bool debugNewThreads = (sTeamFlags & B_TEAM_DEBUG_THREADS);

        if (debugNewThreads)
        {
            debug_debugger_message_data message;

            message.thread_created.new_thread = newThread;

            loader_write_debug_message(B_DEBUGGER_MESSAGE_THREAD_CREATED, message);
        }
    }
}

void loader_debugger_team_created(team_id newTeam)
{
    if (sDebuggerInstalled)
    {
        bool debugNewTeams = (sTeamFlags & B_TEAM_DEBUG_TEAM_CREATION);

        if (debugNewTeams)
        {
            debug_debugger_message_data message;
            message.team_created.new_team = newTeam;

            loader_write_debug_message(B_DEBUGGER_MESSAGE_TEAM_CREATED, message);
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

        if (error < 0)
        {
            // The debugger port is closed.
            // This probably means the debugger is dead.
            sDebuggerInstalled = false;
            return 0;
        }

        switch (op)
        {
            case B_DEBUG_MESSAGE_READ_MEMORY:
            {
                debug_nub_read_memory_reply reply;
                reply.error = B_OK;
                reply.size = 0;

                uintptr_t startAddress = (uintptr_t)data.read_memory.address;
                uintptr_t endAddress = startAddress + data.read_memory.size;

                uintptr_t startPage = startAddress & ~(B_PAGE_SIZE - 1);
                uintptr_t endPage = (endAddress + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

                haiku_area_info areaInfo;
                for (uintptr_t page = startPage; page < endPage;)
                {
                    // TODO: Acquire mmanlock?
                    int areaId = loader_hserver_call_area_for((void*)page);

                    if (areaId < 0)
                    {
                        reply.error = B_BAD_ADDRESS;
                        break;
                    }

                    reply.error = loader_hserver_call_get_area_info(areaId, &areaInfo);
                    if (reply.error != B_OK)
                    {
                        break;
                    }

                    uint32 originalProtection = areaInfo.protection;
                    using kern_set_area_protection_t = status_t (*)(int, uint32);
                    static kern_set_area_protection_t kern_set_area_protection =
                        (kern_set_area_protection_t)loader_runtime_symbol("_moni_set_area_protection");

                    if (!(originalProtection & B_READ_AREA))
                    {
                        reply.error = kern_set_area_protection(areaId, areaInfo.protection | B_READ_AREA);

                        if (reply.error != B_OK)
                        {
                            break;
                        }
                    }

                    uintptr_t startCurrentArea = (uintptr_t)areaInfo.address;
                    uintptr_t endCurrentArea = startCurrentArea + ((areaInfo.size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1));
                    uintptr_t startCopy = std::max(startCurrentArea, startAddress);
                    uintptr_t endCopy = std::min(endCurrentArea, endAddress);

                    memcpy(reply.data + (startCopy - startAddress), (void*)startCopy, endCopy - startCopy);
                    reply.size += endCopy - startCopy;

                    if (!(originalProtection & B_READ_AREA))
                    {
                        kern_set_area_protection(areaId, originalProtection);
                    }

                    page = endCurrentArea;
                }

                if (reply.size > 0)
                {
                    reply.error = B_OK;
                }

                loader_hserver_call_write_port_etc(data.read_memory.reply_port, B_DEBUG_MESSAGE_READ_MEMORY,
                    &reply, sizeof(reply), 0, B_INFINITE_TIMEOUT);
            }
            break;
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