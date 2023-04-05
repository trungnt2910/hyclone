#include <algorithm>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/uio.h>
#include <signal.h>
#include <unistd.h>
#include <unordered_set>

#include "haiku_errors.h"
#include "server_errno.h"
#include "server_native.h"
#include "system.h"

size_t server_read_process_memory(int pid, void* address, void* buffer, size_t size)
{
    if (size == 0)
    {
        return 0;
    }

    struct iovec local_iov = { buffer, size };
    struct iovec remote_iov = { address, size };

    return (size_t)process_vm_readv((pid_t)pid, &local_iov, 1, &remote_iov, 1, 0);
}
size_t server_write_process_memory(int pid, void* address, const void* buffer, size_t size)
{
    if (size == 0)
    {
        return 0;
    }

    struct iovec local_iov = { (void*)buffer, size };
    struct iovec remote_iov = { address, size };

    return (size_t)process_vm_writev((pid_t)pid, &local_iov, 1, &remote_iov, 1, 0);
}

void server_kill_process(int pid)
{
    kill(pid, SIGKILL);
}

void server_close_connection(intptr_t conn_id)
{
    close((int)conn_id);
}

void server_fill_team_info(haiku_team_info* info)
{
    if (info->team == 0)
    {
        info->team = getpid();
    }

    if (info->thread_count == 0)
    {
        info->thread_count =
            std::distance(std::filesystem::directory_iterator("/proc/" + std::to_string(info->team) + "/task"),
                std::filesystem::directory_iterator());
    }

    if (info->image_count == 0)
    {
        std::ifstream fin("/proc/" + std::to_string(info->team) + "/maps");
        std::string line;
        std::unordered_set<std::string> images;
        while (std::getline(fin, line))
        {
            std::stringstream ss(line);
            // address
            ss >> line;
            // permissions
            ss >> line;
            if (line.find('x') == std::string::npos)
            {
                continue;
            }
            // offset
            ss >> line;
            // device
            ss >> line;
            // inode
            ss >> line;
            // pathname
            ss >> line;

            // Probably. To make sure we can also check the headers
            // for the ELF magic.
            if (line.find(".so") != std::string::npos)
            {
                images.insert(line);
            }
        }

        // One for the main binary.
        info->image_count = images.size() + 1;
    }

    if (info->area_count == 0)
    {
        std::ifstream fin("/proc/" + std::to_string(info->team) + "/maps");
        info->area_count = std::count(std::istreambuf_iterator<char>(fin),
            std::istreambuf_iterator<char>(), '\n');
    }

    if (info->argc == 0)
    {
        std::ifstream fin("/proc/" + std::to_string(info->team) + "/cmdline");
        std::string arg;
        std::string args;
        while (std::getline(fin, arg, '\0'))
        {
            ++info->argc;
            if (args.size() < sizeof(info->args))
            {
                args += arg;
                args += ' ';
            }
        }
        memcpy(info->args, args.c_str(), std::min(sizeof(info->args), args.size() + 1));
    }

    if (info->uid == -1)
    {
        struct stat st;
        if (stat(("/proc/" + std::to_string(info->team)).c_str(), &st) != -1)
        {
            auto& mapService = System::GetInstance().GetUserMapService();
            auto lock = mapService.Lock();
            info->uid = mapService.GetUid(st.st_uid);
            info->gid = mapService.GetGid(st.st_gid);
        }
    }
}

void server_fill_thread_info(haiku_thread_info* info)
{
    std::ifstream fin(("/proc/" + std::to_string(info->thread)) + "/stat");

    std::string ignore;

    // pid
    fin >> ignore;
    // executable filename, in parentheses
    std::getline(fin, ignore, ')');

    char state;
    fin >> state;

    switch (state)
    {
        // To-Do: Verify if this translation is correct.
        case 'R':
            info->state = B_THREAD_RUNNING;
            break;
        case 'S':
            info->state = B_THREAD_ASLEEP;
            break;
        case 'D':
            info->state = B_THREAD_WAITING;
            break;
        case 'T':
        case 't':
            info->state = B_THREAD_SUSPENDED;
            break;
        default:
            info->state = B_THREAD_SUSPENDED;
            break;
    }

    // ppid, pgrp, session, tty_nr, tpgid, flags, minflt, cminflt, majflt, cmajflt
    for (int i = 0; i < 10; ++i)
    {
        fin >> ignore;
    }

    uint64_t utime, stime;
    uint64_t clockTicks = sysconf(_SC_CLK_TCK);
    fin >> utime >> stime;

    info->user_time = utime * (uint64_t)(clockTicks / (long double)1000000.0);
    info->kernel_time = stime * (uint64_t)(clockTicks / (long double)1000000.0);

    // 16 -> 27
    for (int i = 0; i < 12; ++i)
    {
        fin >> ignore;
    }

    uint64_t startstack, kstkesp;
    fin >> startstack >> kstkesp;

    if (startstack != 0)
    {
        info->stack_base = (void*)startstack;
    }
    if (kstkesp != 0)
    {
        info->stack_end = (void*)kstkesp;
    }

    // 30 -> 39
    for (int i = 0; i < 10; ++i)
    {
        fin >> ignore;
    }

    int rt_priority;
    fin >> rt_priority;

    info->priority = rt_priority;
}

status_t server_read_stat(const std::filesystem::path& path, haiku_stat& st)
{
    struct stat linux_st;
    if (stat(path.c_str(), &linux_st) == -1)
    {
        return LinuxToB(errno);
    }

    st.st_dev = linux_st.st_dev;
    st.st_ino = linux_st.st_ino;
    st.st_mode = linux_st.st_mode;
    st.st_nlink = linux_st.st_nlink;
    {
        auto& mapService = System::GetInstance().GetUserMapService();
        auto lock = mapService.Lock();
        st.st_uid = mapService.GetUid(linux_st.st_uid);
        st.st_gid = mapService.GetGid(linux_st.st_gid);
    }
    st.st_size = linux_st.st_size;
    st.st_rdev = linux_st.st_rdev;
    st.st_blksize = linux_st.st_blksize;
    st.st_atim.tv_sec = linux_st.st_atim.tv_sec;
    st.st_atim.tv_nsec = linux_st.st_atim.tv_nsec;
    st.st_mtim.tv_sec = linux_st.st_mtim.tv_sec;
    st.st_mtim.tv_nsec = linux_st.st_mtim.tv_nsec;
    st.st_ctim.tv_sec = linux_st.st_ctim.tv_sec;
    st.st_ctim.tv_nsec = linux_st.st_ctim.tv_nsec;
    st.st_blocks = linux_st.st_blocks;
    // Unsupported fields.
    st.st_crtim.tv_sec = 0;
    st.st_crtim.tv_nsec = 0;
    st.st_type = 0;

    return B_OK;
}

status_t server_write_stat(const std::filesystem::path& path, const haiku_stat& st, int statMask)
{
    if (statMask & B_STAT_MODE)
    {
        if (lchmod(path.c_str(), st.st_mode) == -1)
        {
            return LinuxToB(errno);
        }
    }
    if (statMask & (B_STAT_UID | B_STAT_GID))
    {
        int newUid = -1;
        int newGid = -1;
        {
            auto& mapService = System::GetInstance().GetUserMapService();
            auto lock = mapService.Lock();
            if (statMask & B_STAT_UID)
            {
                newUid = mapService.GetHostUid(st.st_uid);
            }
            if (statMask & B_STAT_GID)
            {
                newGid = mapService.GetHostGid(st.st_gid);
            }
        }
        if (lchown(path.c_str(), newUid, newGid) == -1)
        {
            return LinuxToB(errno);
        }
    }
    if (statMask & B_STAT_SIZE)
    {
        if (truncate(path.c_str(), st.st_size) == -1)
        {
            return LinuxToB(errno);
        }
    }
    if (statMask & (B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME))
    {
        struct timespec times[2];
        times[0].tv_nsec = UTIME_OMIT;
        times[1].tv_nsec = UTIME_OMIT;

        if (statMask & B_STAT_ACCESS_TIME)
        {
            if (st.st_atim.tv_nsec == HAIKU_UTIME_NOW)
            {
                times[0].tv_nsec = UTIME_NOW;
            }
            else
            {
                times[0].tv_sec = st.st_atim.tv_sec;
                times[0].tv_nsec = st.st_atim.tv_nsec;
            }
        }

        if (statMask & B_STAT_MODIFICATION_TIME)
        {
            if (st.st_mtim.tv_nsec == HAIKU_UTIME_NOW)
            {
                times[1].tv_nsec = UTIME_NOW;
            }
            else
            {
                times[1].tv_sec = st.st_mtim.tv_sec;
                times[1].tv_nsec = st.st_mtim.tv_nsec;
            }
        }

        if (utimensat(AT_FDCWD, path.c_str(), times, AT_SYMLINK_NOFOLLOW) == -1)
        {
            return LinuxToB(errno);
        }
    }

    return B_OK;
}