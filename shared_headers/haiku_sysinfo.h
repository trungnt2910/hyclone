#ifndef __HAIKU_SYINFO_H__
#define __HAIKU_SYINFO_H__

#include "BeDefs.h"

typedef struct
{
    bigtime_t active_time; /* usec of doing useful work since boot */
    bool enabled;
    uint64 current_frequency;
} haiku_cpu_info;

enum haiku_topology_level_type
{
    B_TOPOLOGY_UNKNOWN,
    B_TOPOLOGY_ROOT,
    B_TOPOLOGY_SMT,
    B_TOPOLOGY_CORE,
    B_TOPOLOGY_PACKAGE
};

enum haiku_cpu_platform
{
    B_CPU_UNKNOWN,
    B_CPU_x86,
    B_CPU_x86_64,
    B_CPU_PPC,
    B_CPU_PPC_64,
    B_CPU_M68K,
    B_CPU_ARM,
    B_CPU_ARM_64,
    B_CPU_ALPHA,
    B_CPU_MIPS,
    B_CPU_SH,
    B_CPU_SPARC,
    B_CPU_RISC_V
};

enum haiku_cpu_vendor
{
    B_CPU_VENDOR_UNKNOWN,
    B_CPU_VENDOR_AMD,
    B_CPU_VENDOR_CYRIX,
    B_CPU_VENDOR_IDT,
    B_CPU_VENDOR_INTEL,
    B_CPU_VENDOR_NATIONAL_SEMICONDUCTOR,
    B_CPU_VENDOR_RISE,
    B_CPU_VENDOR_TRANSMETA,
    B_CPU_VENDOR_VIA,
    B_CPU_VENDOR_IBM,
    B_CPU_VENDOR_MOTOROLA,
    B_CPU_VENDOR_NEC,
    B_CPU_VENDOR_HYGON,
    B_CPU_VENDOR_SUN,
    B_CPU_VENDOR_FUJITSU
};

typedef struct
{
    enum haiku_cpu_platform platform;
} haiku_cpu_topology_root_info;

typedef struct
{
    enum haiku_cpu_vendor vendor;
    uint32 cache_line_size;
} haiku_cpu_topology_package_info;

typedef struct
{
    uint32 model;
    uint64 default_frequency;
} haiku_cpu_topology_core_info;

typedef struct
{
    uint32 id;
    enum haiku_topology_level_type type;
    uint32 level;

    union
    {
        haiku_cpu_topology_root_info root;
        haiku_cpu_topology_package_info package;
        haiku_cpu_topology_core_info core;
    } data;
} haiku_cpu_topology_node_info;

#endif // __HAIKU_SYINFO_H__