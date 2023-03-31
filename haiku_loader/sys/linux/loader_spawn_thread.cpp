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
#include "loader_servercalls.h"
#include "loader_spawn_thread.h"
#include "loader_tls.h"
#include "thread_defs.h"
#include "user_thread_defs.h"

struct loader_trampoline_args
{
    std::atomic<int>* thread_id;
    thread_creation_attributes* attributes;
    bool stack_needs_freeing;
};

static void* loader_pthread_entry_trampoline(void* arg);

static std::unordered_map<int, pthread_t> sHostPthreadObjects;
static std::mutex sHostPthreadObjectsLock;

// To-Do: This function assumes that the stack grows down.
// This may not be true for some architectures other than x86_64.
int loader_spawn_thread(void* arg)
{
    thread_creation_attributes* attributes = (thread_creation_attributes*)arg;

    pthread_attr_t linux_thread_attributes;
    pthread_attr_init(&linux_thread_attributes);

    bool stack_needs_freeing = false;

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
        stack_needs_freeing = true;
    }

    pthread_attr_setstack(&linux_thread_attributes, attributes->stack_address, attributes->stack_size);
    pthread_attr_setguardsize(&linux_thread_attributes, attributes->guard_size);

    sched_param sched;
    sched.sched_priority = attributes->priority;
    pthread_attr_setschedparam(&linux_thread_attributes, &sched);

    pthread_t thread;
    std::atomic<int> thread_id = 0;

    loader_trampoline_args args = {&thread_id, attributes, stack_needs_freeing};

    int result = pthread_create(&thread, &linux_thread_attributes, loader_pthread_entry_trampoline, &args);

    if (result != 0)
    {
        return -result;
    }

    thread_id.wait(0);

    int thread_id_nonatomic = thread_id.load();

    {
        auto lock = std::unique_lock<std::mutex>(sHostPthreadObjectsLock);
        sHostPthreadObjects[thread_id_nonatomic] = thread;
    }

    return thread_id_nonatomic;
}

void loader_exit_thread(int retVal)
{
    // Don't clean anything here, pthread states
    // in the loader will be cleaned when the app calls
    // wait_for_thread
    pthread_exit((void*)(intptr_t)retVal);
}

int loader_wait_for_thread(int thread_id, int* retVal)
{
    pthread_t thread;
    {
        auto lock = std::unique_lock<std::mutex>(sHostPthreadObjectsLock);
        auto it = sHostPthreadObjects.find(thread_id);
        if (it == sHostPthreadObjects.end())
        {
            return -ESRCH;
        }
        thread = it->second;
    }
    // A temporary pointer.
    // Attempting to cast retVal (a int*) to a void** on 64-bit
    // platforms may corrupt data during assignment.
    void* retPtr;
    // Zero if succeeds, positive error code on error.
    int error = pthread_join(thread, &retPtr);
    if (error != 0)
    {
        return -error;
    }
    *retVal = (int)(intptr_t)retPtr;
    {
        auto lock = std::unique_lock<std::mutex>(sHostPthreadObjectsLock);
        sHostPthreadObjects.erase(thread_id);
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

    loader_register_thread(tid, attributes, true);

    haiku_area_info stack_area_info, guard_area_info;
    stack_area_info.address = (void*)((uintptr_t)attributes->stack_address - attributes->stack_size);
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
        guard_area_info.address = (void*)((uintptr_t)attributes->stack_address - attributes->stack_size - attributes->guard_size);
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
    bool stack_needs_freeing = trampoline_args->stack_needs_freeing;

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

    if (stack_needs_freeing)
    {
        munmap(stack_area_info.address, stack_area_info.size);
        if (guard_area_info.size > 0)
        {
            munmap(guard_area_info.address, guard_area_info.size);
        }
    }

    return (void*)(intptr_t)returnValue;
}