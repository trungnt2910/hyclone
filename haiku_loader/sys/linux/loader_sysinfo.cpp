#include <algorithm>
#include <bit>
#include <fstream>
#include <filesystem>
#include <numeric>
#include <set>
#include <string>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "haiku_errors.h"
#include "haiku_team.h"
#include "loader_sysinfo.h"

int64_t get_cpu_count()
{
    return get_nprocs();
}

void get_cpu_info(int index, haiku_cpu_info* info)
{
    const std::filesystem::path sysPath = "/sys/devices/system/cpu/cpu" + std::to_string(index);

    std::ifstream online(sysPath/"online");

    // On some systems, such as WSL, the online file is not present.
    // The CPU is enabled nonetheless.
    info->enabled = true;

    // If the read is successful, this value will be overridden.
    online >> info->enabled;

    std::ifstream freq(sysPath/"cpufreq/cpuinfo_cur_freq");
    if (!freq.is_open())
    {
        // The maximum frequency is more useful than returning 0.
        freq.open(sysPath/"cpufreq/cpuinfo_max_freq");
    }

    info->current_frequency = 0;
    freq >> info->current_frequency;
    // The value is in kHZ.
    info->current_frequency *= 1000;

    struct sysinfo sys_info;
    sysinfo(&sys_info);

    // No way to find real uptime of each CPU.
    // This is better than a stub though.
    info->active_time = sys_info.uptime;
}

int get_cpu_topology_info(haiku_cpu_topology_node_info* info, uint32_t* count)
{
    if (*count == 0)
    {
        return B_OK;
    }

    uint32_t countLeft = *count;

    // This function is mostly undocumented, but after some
    // experiments, it seems:
    // - B_TOPOLOGY_ROOT: The root node of the topology.
    // Contains the architecture of the host.
    // The id field should be set to 0, level set to 3.
    // - B_TOPOLOGY_PACKAGE: One node for each physical CPU.
    // Contains the vendor and the size of the L1 cache.
    // The id field should be set to the index of the CPU,
    // level set to 2.
    // - B_TOPOLOGY_CORE: One node for each logical CPU core.
    // Contains the model and the default frequency.
    // The id field should be set to the index of the CPU core,
    // level set to 1.
    // - B_TOPOLOGY_SMT: Same as B_TOPOLOGY_CORE, but 
    // the level is set to 0 instead of 1.

    // /sys/devices/system/cpu/cpu0/topology/thread_siblings
  
    struct
    {
        uint64_t index;
        uint64_t physical_index;
        uint64_t frequency;
        uint64_t model;
        std::string vendor;
    } current_cpu_info;

    info->id = 0;
    info->level = 3;
    info->type = B_TOPOLOGY_ROOT;

    std::string archString;

    {
        utsname unameInfo;
        if (uname(&unameInfo) == -1)
        {
            return HAIKU_POSIX_ENOSYS;
        }
        archString = unameInfo.machine;
    }

    if (archString == "x86_64")
    {
        info->data.root.platform = B_CPU_x86_64;
    }
    else
    {
        info->data.root.platform = B_CPU_UNKNOWN;
    }

    ++info;
    --countLeft;

    if (countLeft == 0)
    {
        return B_OK;
    }

    std::ifstream fin("/proc/cpuinfo");
    if (!fin.is_open())
    {
        return HAIKU_POSIX_ENOSYS;
    }

    std::string line;
    while (std::getline(fin, line))
    {
        if (line.empty())
        {
            // An empty line. Register the CPU info here.
            std::string topology_path = 
                "/sys/devices/system/cpu/cpu" + std::to_string(current_cpu_info.index) + "/topology/";
            std::ifstream core_siblings(topology_path + "core_siblings");
            if (!core_siblings.is_open())
            {
                return HAIKU_POSIX_ENOSYS;
            }

            uint64_t core_siblings_set = 0;

            core_siblings >> std::hex >> core_siblings_set;

            bool is_first_core = 
                (unsigned)std::countr_zero(core_siblings_set) == current_cpu_info.index;

            if (is_first_core)
            {
                info->id = current_cpu_info.physical_index;
                info->level = 2;
                info->type = B_TOPOLOGY_PACKAGE;

                std::string cache_path = 
                    "/sys/devices/system/cpu/cpu" + std::to_string(current_cpu_info.index) + "/cache/";
                std::ifstream coherency_line_size(topology_path + "coherency_line_size");

                // Many systems do not contain this information,
                // including the dev's WSL.
                if (!coherency_line_size.is_open())
                {
                    info->data.package.cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
                }
                else
                {
                    coherency_line_size >> info->data.package.cache_line_size;
                }

                if (current_cpu_info.vendor == "GenuineIntel")
                {
                    info->data.package.vendor = B_CPU_VENDOR_INTEL;
                }
                else if (current_cpu_info.vendor == "AuthenticAMD")
                {
                    info->data.package.vendor = B_CPU_VENDOR_AMD;
                }
                else
                {
                    info->data.package.vendor = B_CPU_VENDOR_UNKNOWN;
                }

                ++info;
                --countLeft;

                if (countLeft == 0)
                {
                    return B_OK;
                }
            }

            // ID for current core.
            info->id = current_cpu_info.index;
            info->level = 1;
            info->type = B_TOPOLOGY_CORE;

#if defined(__x86_64__) || defined(__i386__)
            asm("cpuid\n\t"
			    : "=a" (info->data.core.model)
			    : "0" (1));
#else
            info->data.core.model = current_cpu_info.model;
#endif
            info->data.core.default_frequency = current_cpu_info.frequency;

            ++info;
            --countLeft;

            if (countLeft == 0)
            {
                return B_OK;
            }

            // ID for current core.
            info->id = current_cpu_info.index;
            info->level = 0;
            info->type = B_TOPOLOGY_SMT;

            ++info;
            --countLeft;

            if (countLeft == 0)
            {
                return B_OK;
            }

            current_cpu_info.index = 0;
            current_cpu_info.physical_index = 0;
            current_cpu_info.vendor.clear();
            current_cpu_info.frequency = 0;
        }
        else
        {
            auto sep = line.find(':');
            if (sep == std::string::npos)
            {
                continue;
            }

            auto key = line.substr(0, sep);
            while (isspace(key.back()))
            {
                key.pop_back();
            }

            if (line.size() < sep + 2)
            {
                // No value for this key.
                continue;
            }

            // After the colon is a space.
            auto value = line.substr(sep + 2);
            while (isspace(value.back()))
            {
                value.pop_back();
            }
            
            if (key == "processor")
            {
                current_cpu_info.index = std::stoull(value);
            }
            else if (key == "vendor_id")
            {
                current_cpu_info.vendor = value;
            }
            else if (key == "cpu MHz")
            {
                current_cpu_info.frequency = (uint64_t)(std::stold(value) * 1000000);
            }
            else if (key == "model")
            {
                current_cpu_info.model = std::stoull(value);
            }
            else if (key == "physical id")
            {
                current_cpu_info.physical_index = std::stoull(value);
            }
        }        
    }

    *count -= countLeft;
    return B_OK;
}

int64_t get_cpu_topology_count()
{
    int64_t count = 0;

    // One root node.
    count += 1;
    // Two node per logical core.
    count += get_nprocs_conf() * 2;

    std::ifstream fin("/proc/cpuinfo");
    if (!fin.is_open())
    {
        // If this file is inaccessible,
        // get_cpu_topology_info can only read
        // the root node.
        return 1;
    }

    std::set<uint64_t> physicalIds;

    std::string line;
    while (std::getline(fin, line))
    {
        auto sep = line.find(':');
        if (sep == std::string::npos)
        {
            continue;
        }

        auto key = line.substr(0, sep);
        while (isspace(key.back()))
        {
            key.pop_back();
        }

        if (line.size() < sep + 2)
        {
            // No value for this key.
            continue;
        }

        // After the colon is a space.
        auto value = line.substr(sep + 2);
        while (isspace(value.back()))
        {
            value.pop_back();
        }
        
        if (key == "physical id")
        {
            physicalIds.insert(std::stoull(value));
        }
    }

    count += physicalIds.size();

    return count;
}

int64_t get_thread_count()
{
    const std::filesystem::path procPath = "/proc";

    int64_t threadCount = 0;

    for (const auto& dirEntry : std::filesystem::directory_iterator(procPath))
    {
        std::string dirName = dirEntry.path().filename();

        // Check if the filename consists of only digits.
        if (std::accumulate(dirName.begin(), dirName.end(), true,
            [](bool a, char b) { return a && (bool)isdigit(b); }))
        {
            std::filesystem::path threadPath = dirEntry.path()/"task";
            threadCount += std::distance(
                std::filesystem::directory_iterator(threadPath),
                std::filesystem::directory_iterator());
        }
    }

    return threadCount;
}

int64_t get_process_count()
{
    const std::filesystem::path procPath = "/proc";

    return (int64_t)std::count_if(
        std::filesystem::directory_iterator(procPath),
        std::filesystem::directory_iterator(),
        [](const std::filesystem::directory_entry& entry)
        {
            std::string dirName = entry.path().filename();
            return std::accumulate(dirName.begin(), dirName.end(), true,
                [](bool a, char b) { return a && (bool)isdigit(b); });
        });
}

int64_t get_page_size()
{
    return (int64_t)sysconf(_SC_PAGE_SIZE);
}

int64_t get_cached_memory_size()
{
    std::ifstream fin("/proc/meminfo");
    if (!fin.is_open())
        return 0;
    std::string line;
    while (std::getline(fin, line))
    {
        if (line.find("Cached:") != std::string::npos)
        {
            std::size_t pos = sizeof("Cached:");
            while (pos < line.size() && line[pos] == ' ')
                ++pos;
            // Value is in kilobytes.
            return std::stoll(line.substr(pos)) * 1024;
        }
    }
    return 0;
}

// This could be totally done in Monika, but using raw syscalls
// is hell. Here we're blessed by Linux's C++ library.
int64_t get_max_threads()
{
    std::ifstream fin("/proc/sys/kernel/threads-max");
    if (!fin.is_open())
        return -1;
    int64_t max_threads = -1;
    fin >> max_threads;
    return max_threads;
}

int64_t get_max_sems()
{
    std::ifstream fin("/proc/sys/kernel/sem");
    if (!fin.is_open())
        return -1;
    int64_t SEMMSL = -1, SEMMNS = -1;
    fin >> SEMMSL >> SEMMNS;
    return SEMMNS;
}

int64_t get_max_procs()
{
    std::ifstream fin("/proc/sys/kernel/pid_max");
    if (!fin.is_open())
        return -1;
    int64_t pid_max = -1;
    fin >> pid_max;
    return pid_max;
}

int get_process_usage(int pid, int who, team_usage_info* info)
{
    if (pid == B_CURRENT_TEAM)
    {
        struct rusage usage;
        int linuxWho;
        switch (who)
        {
            case B_TEAM_USAGE_SELF:
                linuxWho = RUSAGE_SELF;
                break;
            case B_TEAM_USAGE_CHILDREN:
                linuxWho = RUSAGE_CHILDREN;
                break;
            default:
                return B_BAD_VALUE;
        }

        if (getrusage(linuxWho, &usage) != 0)
        {
            return B_BAD_VALUE;   
        }

        info->user_time = usage.ru_utime.tv_sec * 1000000 + usage.ru_utime.tv_usec;
        info->kernel_time = usage.ru_stime.tv_sec * 1000000 + usage.ru_stime.tv_usec;

        return B_OK;
    }

    if (who != B_TEAM_USAGE_SELF)
    {
        // We don't support this.
        return B_BAD_VALUE;
    }

    std::string temp = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream fin(temp.c_str());

    if (!fin)
    {
        return HAIKU_POSIX_ENOSYS;
    }

    // pid
    fin >> temp;

    // The filename of the executable, in parentheses.
    do
    {
        fin >> temp;
    }
    while (temp.back() != ')');

    // 11 irrelevant members.
    for (int i = 0; i < 11; ++i)
    {
        fin >> temp;
    }

    long clockTicks = sysconf(_SC_CLK_TCK);
    uint64_t utime, stime;
    fin >> utime >> stime;

    // To-Do: overflow?
    info->user_time = utime * 1000 / clockTicks;
    info->kernel_time = stime * 1000 / clockTicks;

    return B_OK;
}