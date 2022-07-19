#include <sys/sysinfo.h>
#include <string.h>

#include "BeDefs.h"
#include "errno_conversion.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "linux_debug.h"
#include "linux_syscall.h"

typedef struct
{
    bigtime_t boot_time; /* time of boot (usecs since 1/1/1970) */

    uint32 cpu_count; /* number of cpus */

    uint64 max_pages;  /* total # of accessible pages */
    uint64 used_pages; /* # of accessible pages in use */
    uint64 cached_pages;
    uint64 block_cache_pages;
    uint64 ignored_pages; /* # of ignored/inaccessible pages */

    uint64 needed_memory;
    uint64 free_memory;

    uint64 max_swap_pages;
    uint64 free_swap_pages;

    uint32 page_faults; /* # of page faults */

    uint32 max_sems;
    uint32 used_sems;

    uint32 max_ports;
    uint32 used_ports;

    uint32 max_threads;
    uint32 used_threads;

    uint32 max_teams;
    uint32 used_teams;

    char kernel_name[B_FILE_NAME_LENGTH];
    char kernel_build_date[B_OS_NAME_LENGTH];
    char kernel_build_time[B_OS_NAME_LENGTH];

    int64 kernel_version;
    uint32 abi; /* the system API */
} haiku_system_info;

#define B_HAIKU_ABI_GCC_4 0x00040000

#define stringify_macro(a) stringify(a)
#define stringify(a) #a

extern "C"
{

status_t MONIKA_EXPORT _kern_get_system_info(haiku_system_info *info)
{
    struct sysinfo linux_sysinfo;
    long result = LINUX_SYSCALL1(__NR_sysinfo, &linux_sysinfo);

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    int page_size = (int)GET_HOSTCALLS()->get_page_size();

    info->boot_time = linux_sysinfo.uptime;
    info->cpu_count = GET_HOSTCALLS()->get_cpu_count();
    info->max_pages = linux_sysinfo.totalram * linux_sysinfo.mem_unit / page_size;
    info->used_pages = (linux_sysinfo.totalram - linux_sysinfo.freeram) * linux_sysinfo.mem_unit / page_size;
    info->cached_pages = GET_HOSTCALLS()->get_cached_memory_size() / page_size;
    info->block_cache_pages = 0;
    info->ignored_pages = 0;

    info->needed_memory = (linux_sysinfo.totalram - linux_sysinfo.freeram) * linux_sysinfo.mem_unit;
    info->free_memory = linux_sysinfo.freeram * linux_sysinfo.mem_unit;

    info->max_swap_pages = linux_sysinfo.totalswap * linux_sysinfo.mem_unit / page_size;
    info->free_swap_pages = linux_sysinfo.freeswap * linux_sysinfo.mem_unit / page_size;

    info->page_faults = 0;

    info->max_sems = GET_HOSTCALLS()->get_max_sems();
    info->used_sems = 0;

    // This is something we might need to call the kernel server.
    info->max_ports = 0;
    info->used_ports = 0;

    info->max_threads = GET_HOSTCALLS()->get_max_threads();
    // linux_sysinfo.procs actually returns the number of threads.
    info->used_threads = linux_sysinfo.procs;

    info->max_teams = GET_HOSTCALLS()->get_max_procs();
    info->used_teams = GET_HOSTCALLS()->get_process_count();

    strncpy(info->kernel_name,
        "kernel_"
        stringify_macro(HYCLONE_ARCH)"_"
        "hyclone_monika_"
        stringify_macro(HYCLONE_HOST),
        sizeof(info->kernel_name));
    strncpy(info->kernel_build_date, __DATE__, sizeof(info->kernel_build_date));
    strncpy(info->kernel_build_time, __TIME__, sizeof(info->kernel_build_time));

    info->kernel_version = 0x1;
    info->abi = B_HAIKU_ABI_GCC_4;

    return B_OK;
}

int MONIKA_EXPORT _kern_is_computer_on()
{
    // Seriously? Who thought of this syscall?
    return 1;
}

status_t MONIKA_EXPORT _kern_get_cpu_topology_info(haiku_cpu_topology_node_info *topologyInfos, uint32 *topologyInfoCount)
{
    if (topologyInfos == NULL)
    {
        *topologyInfoCount = (uint32)GET_HOSTCALLS()->get_cpu_topology_count();
        return B_OK;
    }
    else
    {
        return GET_HOSTCALLS()->get_cpu_topology_info(topologyInfos, topologyInfoCount);
    }
}

status_t MONIKA_EXPORT _kern_get_cpu_info(uint32 firstCPU, uint32 cpuCount, haiku_cpu_info *cpuInfos)
{
    for (uint32 cpuIndex = firstCPU, i = 0; i < cpuCount; ++cpuIndex, ++i)
    {
        GET_HOSTCALLS()->get_cpu_info(i, cpuInfos + i);
    }

    return B_OK;
}

}