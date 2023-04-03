#include <algorithm>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/uio.h>
#include <signal.h>
#include <unistd.h>
#include <unordered_set>

#include "server_native.h"
#include "system.h"

size_t server_read_process_memory(int pid, void* address, void* buffer, size_t size)
{
    struct iovec local_iov = { buffer, size };
    struct iovec remote_iov = { address, size };

    return (size_t)process_vm_readv((pid_t)pid, &local_iov, 1, &remote_iov, 1, 0);
}
size_t server_write_process_memory(int pid, void* address, const void* buffer, size_t size)
{
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