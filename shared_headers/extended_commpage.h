#ifndef __HAIKU_COMMPAGE_H__
#define __HAIKU_COMMPAGE_H__

#include <cstddef>
#include <cstdint>
#include "BeDefs.h"
#include "commpage_defs.h"
#include "haiku_errors.h"
#include "haiku_sysinfo.h"
#include "servercalls.h"

struct hostcalls
{
    // TLS
    int32_t (*tls_allocate)();
    void* (*tls_get)(int32_t index);
    void** (*tls_address)(int32_t index);
    void (*tls_set)(int32_t index, void* value);

    // System configuration
    int64_t (*get_cpu_count)();
    void (*get_cpu_info)(int index, haiku_cpu_info* info);
    int (*get_cpu_topology_info)(haiku_cpu_topology_node_info* info, uint32_t* count);
    int64_t (*get_cpu_topology_count)();
    int64_t (*get_thread_count)();
    int64_t (*get_process_count)();
    int64_t (*get_page_size)();
    int64_t (*get_cached_memory_size)();
    int64_t (*get_max_threads)();
    int64_t (*get_max_sems)();
    int64_t (*get_max_procs)();
    int (*get_process_usage)(int pid, team_usage_info* info);

    // Memory reservation
    void (*lock_reserved_range_data)();
    void (*unlock_reserved_range_data)();
    void (*register_reserved_range)(void* address, size_t size);
    void (*unregister_reserved_range)(void* address, size_t size);
    void (*map_reserved_range)(void* address, size_t size);
    void (*unmap_reserved_range)(void* address, size_t size);
    bool (*is_in_reserved_range)(void* address, size_t size);
    bool (*collides_with_reserved_range)(void* address, size_t size);
    size_t (*reserved_range_longest_mappable_from)(void* address, size_t maxSize);
    size_t (*next_reserved_range)(void* address, void** nextAddress);
    size_t (*next_reserved_range_mapping)(void* address, void** nextAddress);

    // Lock
    void (*lock_subsystem)(int* subsystemId);
    void (*unlock_subsystem)(int subsystemId);

    // Id map
    void* (*idmap_create)();
    void (*idmap_destroy)(void* idmap);
    int (*idmap_add)(void* idmap, void* data);
    void* (*idmap_get)(void* idmap, int id);
    void (*idmap_remove)(void* idmap, int id);

    // Fork
    int (*fork)();
    int (*exec)(const char* path, const char* const* flatArgs, size_t flatArgsSize, int argc, int envc, int umask);
    int (*spawn)(const char* path, const char* const* flatArgs, size_t flatArgsSize, int argc, int envc, int priority, int flags, int errorPort, int errorToken);

    // Thread
    int (*spawn_thread)(void* thread_creation_attributes);
    void (*exit_thread)(int status);
    int (*wait_for_thread)(int thread, int* status);

    // Mutex
    int (*mutex_lock)(int32_t* mutex, const char* name, uint32_t flags, int64_t timeout);
    int (*mutex_unlock)(int32_t* mutex, uint32_t flags);
    int (*mutex_switch_lock)(int32_t* fromMutex, int32_t* toMutex, const char* name, uint32_t flags, int64_t timeout);

    // Signals
    int (*get_sigrtmin)();
    int (*get_sigrtmax)();

    // Debugger
    bool (*is_debugger_present)();

    // Readdir
    void (*opendir)(int fd);
    void (*closedir)(int fd);
    int (*readdir)(int fd, void* buffer, size_t bufferSize, int maxCount);
    void (*rewinddir)(int fd);

    // Time
    int64_t (*system_time)();
    int64_t (*system_time_nsecs)();
    int64_t (*system_time_offset)();
    size_t (*system_timezone)(int* offset, char* buffer, size_t bufferSize);

    // Vchroot
    size_t (*vchroot_expand)(const char* path, char* hostPath, size_t size);
    size_t (*vchroot_unexpand)(const char* hostPath, char* path, size_t size);
    size_t (*vchroot_expandat)(int fd, const char* path, char* hostPath, size_t size);
    size_t (*vchroot_unexpandat)(int fd, const char* path, char* hostPath, size_t size);
    size_t (*vchroot_expandlink)(const char* path, char* hostPath, size_t size);
    size_t (*vchroot_expandlinkat)(int fd, const char* path, char* hostPath, size_t size);

    // Hooks
    void (*at_exit)(int value);
    int (*printf)(const char* format, ...);
};

// After the real Haiku commpage, we put a page
// containing HyClone private data.
#define EXTENDED_COMMPAGE_SIZE 4096
#define EXTENDED_COMMPAGE_HOSTCALLS_OFFSET COMMPAGE_SIZE
#define EXTENDED_COMMPAGE_SERVERCALLS_OFFSET (EXTENDED_COMMPAGE_HOSTCALLS_OFFSET + sizeof(hostcalls))

#define HYCLONE_SUBSYSTEM_LOCK_UNINITIALIZED (-1)

extern "C" void* __gCommPageAddress;

#define GET_HOSTCALLS() \
    ((struct hostcalls *)((char *)__gCommPageAddress + EXTENDED_COMMPAGE_HOSTCALLS_OFFSET))

#define GET_SERVERCALLS() \
    ((struct servercalls *)((char *)__gCommPageAddress + EXTENDED_COMMPAGE_SERVERCALLS_OFFSET))

#define CHECK_COMMPAGE() \
    if (__gCommPageAddress == NULL) \
    { \
        return HAIKU_POSIX_ENOSYS; \
    }

#endif // __HAIKU_COMMPAGE_H__