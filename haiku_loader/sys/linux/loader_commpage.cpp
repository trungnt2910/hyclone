#include <cstddef>
#include <cstdio>
#include <cstring>

#include <sys/mman.h>

#include "BeDefs.h"
#include "commpage_defs.h"
#include "extended_commpage.h"
#include "haiku_image.h"
#include "haiku_tls.h"
#include "loader_debugger.h"
#include "loader_exec.h"
#include "loader_fork.h"
#include "loader_idmap.h"
#include "loader_lock.h"
#include "loader_mutex.h"
#include "loader_protectedfd.h"
#include "loader_readdir.h"
#include "loader_reservedrange.h"
#include "loader_semaphore.h"
#include "loader_servercalls.h"
#include "loader_signal.h"
#include "loader_spawn.h"
#include "loader_spawn_thread.h"
#include "loader_sysinfo.h"
#include "loader_systemtime.h"
#include "loader_tls.h"
#include "loader_vchroot.h"
#include "real_time_data.h"

#include "loader_commpage.h"

void* loader_allocate_commpage()
{
    void* commpage = mmap(NULL, COMMPAGE_SIZE + EXTENDED_COMMPAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (commpage == MAP_FAILED)
        return NULL;

    char* commpageStorage = (char*)((uintptr_t)commpage + sizeof(uint64_t) * COMMPAGE_TABLE_ENTRIES);
    real_time_data* realTimeData = (real_time_data*)commpageStorage;
    commpageStorage += sizeof(real_time_data);

    ((uint64_t*)commpage)[COMMPAGE_ENTRY_MAGIC] = COMMPAGE_SIGNATURE;
	((uint64_t*)commpage)[COMMPAGE_ENTRY_VERSION] = COMMPAGE_VERSION;
    ((uint64_t*)commpage)[COMMPAGE_ENTRY_REAL_TIME_DATA] =
        (uint64_t)((uintptr_t)realTimeData - (uintptr_t)commpage);
#ifdef __x86_64__
    // Stubbed.
    realTimeData->arch_data.system_time_conversion_factor = 0;
    realTimeData->arch_data.system_time_offset = 0;
#else
#warning commpage time data not set up for this architecture.
#endif

    hostcalls* hostcalls_ptr = (hostcalls*)((uint8_t*)commpage + EXTENDED_COMMPAGE_HOSTCALLS_OFFSET);
    hostcalls_ptr->tls_allocate = tls_allocate;
    hostcalls_ptr->tls_get = tls_get;
    hostcalls_ptr->tls_address = tls_address;
    hostcalls_ptr->tls_set = tls_set;

    hostcalls_ptr->get_cpu_count = get_cpu_count;
    hostcalls_ptr->get_cpu_info = get_cpu_info;
    hostcalls_ptr->get_cpu_topology_info = get_cpu_topology_info;
    hostcalls_ptr->get_cpu_topology_count = get_cpu_topology_count;
    hostcalls_ptr->get_thread_count = get_thread_count;
    hostcalls_ptr->get_process_count = get_process_count;
    hostcalls_ptr->get_page_size = get_page_size;
    hostcalls_ptr->get_cached_memory_size = get_cached_memory_size;
    hostcalls_ptr->get_max_threads = get_max_threads;
    hostcalls_ptr->get_max_sems = get_max_sems;
    hostcalls_ptr->get_max_procs = get_max_procs;
    hostcalls_ptr->get_process_usage = get_process_usage;

    hostcalls_ptr->lock_reserved_range_data = loader_lock_reserved_range_data;
    hostcalls_ptr->unlock_reserved_range_data = loader_unlock_reserved_range_data;
    hostcalls_ptr->register_reserved_range = loader_register_reserved_range;
    hostcalls_ptr->unregister_reserved_range = loader_unregister_reserved_range;
    hostcalls_ptr->map_reserved_range = loader_map_reserved_range;
    hostcalls_ptr->unmap_reserved_range = loader_unmap_reserved_range;
    hostcalls_ptr->is_in_reserved_range = loader_is_in_reserved_range;
    hostcalls_ptr->collides_with_reserved_range = loader_collides_with_reserved_range;
    hostcalls_ptr->reserved_range_longest_mappable_from = loader_reserved_range_longest_mappable_from;
    hostcalls_ptr->next_reserved_range = loader_next_reserved_range;
    hostcalls_ptr->next_reserved_range_mapping = loader_next_reserved_range_mapping;

    hostcalls_ptr->lock_subsystem = loader_lock_subsystem;
    hostcalls_ptr->unlock_subsystem = loader_unlock_subsystem;

    hostcalls_ptr->idmap_create = loader_idmap_create;
    hostcalls_ptr->idmap_destroy = loader_idmap_destroy;
    hostcalls_ptr->idmap_add = loader_idmap_add;
    hostcalls_ptr->idmap_get = loader_idmap_get;
    hostcalls_ptr->idmap_remove = loader_idmap_remove;

    hostcalls_ptr->fork = loader_fork;
    hostcalls_ptr->exec = loader_exec;
    hostcalls_ptr->spawn = loader_spawn;

    hostcalls_ptr->spawn_thread = loader_spawn_thread;
    hostcalls_ptr->exit_thread = loader_exit_thread;
    hostcalls_ptr->wait_for_thread = loader_wait_for_thread;

    hostcalls_ptr->mutex_lock = loader_mutex_lock;
    hostcalls_ptr->mutex_unlock = loader_mutex_unlock;
    hostcalls_ptr->mutex_switch_lock = loader_mutex_switch_lock;

    hostcalls_ptr->realtime_sem_open = loader_realtime_sem_open;

    hostcalls_ptr->get_sigrtmin = loader_get_sigrtmin;
    hostcalls_ptr->get_sigrtmax = loader_get_sigrtmax;

    hostcalls_ptr->is_debugger_present = loader_is_debugger_present;
    hostcalls_ptr->debugger_pre_syscall = loader_debugger_pre_syscall;
    hostcalls_ptr->debugger_post_syscall = loader_debugger_post_syscall;

    hostcalls_ptr->opendir = loader_opendir;
    hostcalls_ptr->closedir = loader_closedir;
    hostcalls_ptr->readdir = loader_readdir;
    hostcalls_ptr->rewinddir = loader_rewinddir;
    hostcalls_ptr->dupdir = loader_dupdir;

    hostcalls_ptr->system_time = loader_system_time;
    hostcalls_ptr->system_time_nsecs = loader_system_time_nsecs;
    hostcalls_ptr->system_time_offset = loader_system_time_offset;
    hostcalls_ptr->system_timezone = loader_system_timezone;

    hostcalls_ptr->vchroot_expand = loader_vchroot_expand;
    hostcalls_ptr->vchroot_unexpand = loader_vchroot_unexpand;
    hostcalls_ptr->vchroot_expandat = loader_vchroot_expandat;
    hostcalls_ptr->vchroot_unexpandat = loader_vchroot_unexpandat;
    hostcalls_ptr->vchroot_expandlink = loader_vchroot_expandlink;
    hostcalls_ptr->vchroot_expandlinkat = loader_vchroot_expandlinkat;

    hostcalls_ptr->is_protected_fd = loader_is_protected_fd;

    hostcalls_ptr->at_exit = NULL;
    hostcalls_ptr->printf = loader_dprintf;
    hostcalls_ptr->snprintf = snprintf;

    servercalls* servercalls_ptr = (servercalls*)((uint8_t*)commpage + EXTENDED_COMMPAGE_SERVERCALLS_OFFSET);
    #define HYCLONE_SERVERCALL0(name) servercalls_ptr->name = loader_hserver_call_##name;
    #define HYCLONE_SERVERCALL1(name, arg1) HYCLONE_SERVERCALL0(name)
    #define HYCLONE_SERVERCALL2(name, arg1, arg2) HYCLONE_SERVERCALL1(name, arg1)
    #define HYCLONE_SERVERCALL3(name, arg1, arg2, arg3) HYCLONE_SERVERCALL2(name, arg1, arg2)
    #define HYCLONE_SERVERCALL4(name, arg1, arg2, arg3, arg4) HYCLONE_SERVERCALL3(name, arg1, arg2, arg3)
    #define HYCLONE_SERVERCALL5(name, arg1, arg2, arg3, arg4, arg5) HYCLONE_SERVERCALL4(name, arg1, arg2, arg3, arg4)
    #define HYCLONE_SERVERCALL6(name, arg1, arg2, arg3, arg4, arg5, arg6) HYCLONE_SERVERCALL5(name, arg1, arg2, arg3, arg4, arg5)
    #include "servercall_defs.h"
    #undef HYCLONE_SERVERCALL0
    #undef HYCLONE_SERVERCALL1
    #undef HYCLONE_SERVERCALL2
    #undef HYCLONE_SERVERCALL3
    #undef HYCLONE_SERVERCALL4
    #undef HYCLONE_SERVERCALL5
    #undef HYCLONE_SERVERCALL6

    mprotect(commpage, COMMPAGE_SIZE, PROT_READ);

    haiku_extended_image_info info;
    memset(&info, 0, sizeof(info));

    info.basic_info.type = B_SYSTEM_IMAGE;
    strncpy(info.basic_info.name, "commpage", B_PATH_NAME_LENGTH);
    info.basic_info.text = commpage;
    info.basic_info.text_size = COMMPAGE_SIZE;
    info.basic_info.data = (uint8_t*)commpage + COMMPAGE_SIZE;
    info.basic_info.data_size = EXTENDED_COMMPAGE_SIZE;

    loader_hserver_call_register_image(&info, sizeof(info));

    return commpage;
}

void loader_free_commpage(void* commpage)
{
    munmap(commpage, COMMPAGE_SIZE + EXTENDED_COMMPAGE_SIZE);
}
