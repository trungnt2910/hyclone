#include <atomic>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <pthread.h>
#include <string>
#include <sys/mman.h>
#include <unistd.h>
#include <unordered_map>

#include "haiku_area.h"
#include "haiku_errors.h"
#include "haiku_tls.h"
#include "haiku_thread.h"
#include "loader_debugger.h"
#include "loader_servercalls.h"
#include "loader_spawn_thread.h"
#include "loader_tls.h"
#include "thread_defs.h"
#include "user_thread_defs.h"

struct loader_trampoline_args
{
    std::atomic<int>* thread_id;
    thread_creation_attributes* attributes;
};

struct ThreadInfo
{
    pthread_t thread;
    void* stackEnd;
    size_t guardSize;
};

static void* loader_pthread_entry_trampoline(void* arg);

static std::unordered_map<int, ThreadInfo> sHostPthreadObjects;
static std::mutex sHostPthreadObjectsLock;

// To-Do: This function assumes that the stack grows down.
// This may not be true for some architectures other than x86_64.
int loader_spawn_thread(void* arg)
{
    thread_creation_attributes* attributes = (thread_creation_attributes*)arg;

    pthread_attr_t linux_thread_attributes;
    pthread_attr_init(&linux_thread_attributes);

    ThreadInfo threadInfo;
    memset(&threadInfo, 0, sizeof(threadInfo));

    if (attributes->stack_address == NULL)
    {
        size_t guard_size = attributes->guard_size;
        size_t stack_size = attributes->stack_size;
        const int stack_prot = PROT_READ | PROT_WRITE;
        void* stack_end = mmap(NULL, stack_size + guard_size,
            (guard_size == 0) ? stack_prot : PROT_NONE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
        if (stack_end == MAP_FAILED)
        {
            return -EAGAIN;
        }

        if (guard_size != 0)
        {
            void* guard_end = (void*)((uintptr_t)stack_end + guard_size);
            mprotect(guard_end, stack_size, stack_prot);
        }

        attributes->stack_address = (void*)((uintptr_t)stack_end + guard_size);
        threadInfo.stackEnd = stack_end;
        threadInfo.guardSize = guard_size;
    }

    int status = pthread_attr_setstack(&linux_thread_attributes, attributes->stack_address, attributes->stack_size);
    if (status != 0)
    {
        return -status;
    }

    status = pthread_attr_setguardsize(&linux_thread_attributes, attributes->guard_size);
    if (status != 0)
    {
        return -status;
    }

    // At this point, according to POSIX:
    // After a successful call to pthread_attr_setstack(), the storage area specified by the stackaddr parameter
    // is under the control of the implementation, as described in Use of Application-Managed Thread Stacks.
    // As for the guard page:
    // If the stackaddr attribute has been set (that is, the caller is allocating and managing its own thread stacks),
    // the guardsize attribute shall be ignored and no protection shall be provided by the implementation.
    // It is the responsibility of the application to manage stack overflow along with stack allocation and
    // management in this case.
    //
    // We therefore store the stack end and the guard page size for deallocation.

    sched_param sched;
    sched.sched_priority = attributes->priority;
    pthread_attr_setschedparam(&linux_thread_attributes, &sched);

    std::atomic<int> threadId = 0;

    loader_trampoline_args args = {&threadId, attributes};

    int result = pthread_create(&threadInfo.thread, &linux_thread_attributes, loader_pthread_entry_trampoline, &args);

    pthread_attr_destroy(&linux_thread_attributes);

    if (result != 0)
    {
        return -result;
    }

    threadId.wait(0);

    {
        auto lock = std::unique_lock<std::mutex>(sHostPthreadObjectsLock);
        sHostPthreadObjects[threadId] = threadInfo;
    }

    loader_debugger_thread_created(threadId);

    return threadId;
}

void loader_exit_thread(int retVal)
{
    // Don't clean anything here, pthread states
    // in the loader will be cleaned when the app calls
    // wait_for_thread
    pthread_exit((void*)(intptr_t)retVal);
}

int loader_wait_for_thread(int threadId, uint32_t flags, int64_t timeout, int* retVal)
{
    ThreadInfo threadInfo;
    {
        auto lock = std::unique_lock<std::mutex>(sHostPthreadObjectsLock);
        auto it = sHostPthreadObjects.find(threadId);
        if (it == sHostPthreadObjects.end())
        {
            return -ESRCH;
        }
        threadInfo = it->second;
    }

    // A temporary pointer.
    // Attempting to cast retVal (a int*) to a void** on 64-bit
    // platforms may corrupt data during assignment.
    void* retPtr;
    // Zero if succeeds, positive error code on error.
    int error = EINVAL;

    bool absolute = flags & B_ABSOLUTE_TIMEOUT;
    bool relative = flags & B_RELATIVE_TIMEOUT;

    // TODO: This is getting messy, maybe we should send this to hyclone_server.
    if (timeout == B_INFINITE_TIMEOUT || (!absolute && !relative))
    {
        error = pthread_join(threadInfo.thread, &retPtr);
    }
    else
    {
        if (absolute)
        {
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);

            timeout -= tp.tv_nsec / 1000;
            timeout -= tp.tv_sec * 1'000'000;
        }

        struct timespec abs;
        clock_gettime(CLOCK_REALTIME, &abs);

        abs.tv_sec += timeout / 1'000'000;
        abs.tv_nsec += (timeout % 1'000'000) * 1000;
        abs.tv_sec += abs.tv_nsec / 1'000'000'000;
        abs.tv_nsec %= 1'000'000'000;

        error = pthread_timedjoin_np(threadInfo.thread, &retPtr, &abs);
    }

    if (error != 0)
    {
        return -error;
    }
    if (retVal != NULL)
    {
        *retVal = (int)(intptr_t)retPtr;
    }
    {
        auto lock = std::unique_lock<std::mutex>(sHostPthreadObjectsLock);
        threadInfo = sHostPthreadObjects[threadId];
        sHostPthreadObjects.erase(threadId);
    }
    if (threadInfo.guardSize != 0)
    {
        munmap(threadInfo.stackEnd, threadInfo.guardSize);
    }
    return 0;
}

bool loader_register_thread(int tid, const thread_creation_attributes* attributes, bool suspended)
{
    haiku_thread_info thread_info;
    memset(&thread_info, 0, sizeof(thread_info));

    thread_info.team = getpid();
    thread_info.thread = (tid == -1) ? getpid() : tid;

    if (attributes != NULL)
    {
        strncpy(thread_info.name, attributes->name, sizeof(thread_info.name));
        thread_info.name[sizeof(thread_info.name) - 1] = '\0';
        thread_info.stack_base = attributes->stack_address;
        // Stack grows down.
        thread_info.stack_end = (uint8_t*)attributes->stack_address + attributes->stack_size;
        thread_info.priority = attributes->priority;
    }
    else
    {
        size_t stack_size;
        thread_info.name[0] = '\0';
        pthread_attr_t attr;
        pthread_getattr_np(pthread_self(), &attr);
        pthread_attr_getstack(&attr, &thread_info.stack_base, &stack_size);
        thread_info.stack_end = (uint8_t*)thread_info.stack_base + stack_size;
        sched_param sched;
        pthread_attr_getschedparam(&attr, &sched);
        thread_info.priority = sched.sched_priority;
        pthread_attr_destroy(&attr);
    }

    if (suspended)
    {
        thread_info.state = B_THREAD_SUSPENDED;
    }
    else
    {
        thread_info.state = B_THREAD_RUNNING;
    }

    return loader_hserver_call_register_thread_info(&thread_info) == B_OK;
}

void* loader_pthread_entry_trampoline(void* arg)
{
    auto trampoline_args = (loader_trampoline_args*)arg;
    thread_creation_attributes* attributes = trampoline_args->attributes;

    int tid = gettid();

    loader_init_tls();

    user_thread* user_thread = (struct user_thread*)tls_get(TLS_USER_THREAD_SLOT);
    user_thread->flags = attributes->flags;
    user_thread->pthread = attributes->pthread;

    loader_init_servercalls();

    if (attributes->name == NULL)
    {
        attributes->name = "user thread";
    }
    pthread_setname_np(pthread_self(), attributes->name);

    loader_register_thread(tid, attributes, true);

    haiku_area_info stack_area_info, guard_area_info;
    stack_area_info.address = attributes->stack_address;
    stack_area_info.size = attributes->stack_size;
    stack_area_info.protection = B_READ_AREA | B_WRITE_AREA | B_STACK_AREA;
    stack_area_info.lock = 0;
    {
        std::string area_name = std::string(attributes->name) + "_" + std::to_string(tid) + "_stack";
        strncpy(stack_area_info.name, area_name.c_str(), sizeof(stack_area_info.name));
    }
    stack_area_info.area = loader_hserver_call_register_area(&stack_area_info, REGION_PRIVATE_MAP);

    if (attributes->guard_size > 0)
    {
        guard_area_info.address = (void*)((uintptr_t)attributes->stack_address - attributes->guard_size);
        guard_area_info.size = attributes->guard_size;
        guard_area_info.protection = 0;
        guard_area_info.lock = 0;
        {
            std::string area_name = std::string(attributes->name) + "_" + std::to_string(tid) + "_stack_guard";
            strncpy(guard_area_info.name, area_name.c_str(), sizeof(guard_area_info.name));
        }
        guard_area_info.area = loader_hserver_call_register_area(&guard_area_info, REGION_PRIVATE_MAP);
    }
    else
    {
        guard_area_info.size = 0;
    }

    decltype(attributes->entry) entry_point = attributes->entry;
    void* args1 = attributes->args1;
    void* args2 = attributes->args2;

    // From here, trampoline_args may be invalidated.
    trampoline_args->thread_id->store(tid);
    trampoline_args->thread_id->notify_all();

    // Every Haiku thread starts suspended.
    loader_hserver_call_wait_for_resume();

    int returnValue = entry_point(args1, args2);

    loader_hserver_call_unregister_area(stack_area_info.area);
    if (attributes->guard_size > 0)
    {
        loader_hserver_call_unregister_area(guard_area_info.area);
    }

    return (void*)(intptr_t)returnValue;
}
