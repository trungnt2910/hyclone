#ifndef __LOADER_SYSINFO_H__
#define __LOADER_SYSINFO_H__

#include <cstdint>
#include "BeDefs.h"
#include "haiku_sysinfo.h"

__attribute__((weak))
extern int64_t get_cpu_count();

__attribute__((weak))
extern void get_cpu_info(int index, haiku_cpu_info* info);

__attribute__((weak))
extern int get_cpu_topology_info(haiku_cpu_topology_node_info* info, uint32_t* count);

__attribute__((weak))
extern int64_t get_cpu_topology_count();

__attribute__((weak))
extern int64_t get_thread_count();

__attribute__((weak))
extern int64_t get_process_count();

__attribute__((weak))
extern int64_t get_page_size();

__attribute__((weak))
extern int64_t get_cached_memory_size();

__attribute__((weak))
extern int64_t get_max_threads();

__attribute__((weak))
extern int64_t get_max_sems();

__attribute__((weak))
extern int64_t get_max_procs();

__attribute__((weak))
extern int get_process_usage(int pid, int who, team_usage_info* info);

#endif // __LOADER_SYSINFO_H__