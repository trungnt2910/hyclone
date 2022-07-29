#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/uio.h>
#include <signal.h>
#include <unistd.h>

#include "server_native.h"

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

void server_fill_thread_info(haiku_thread_info* info)
{
    std::ifstream fin(("/proc/" + std::to_string(info->thread)) + "/status");

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

    info->stack_base = (void*)startstack;
    info->stack_end = (void*)kstkesp;

    // 30 -> 39
    for (int i = 0; i < 10; ++i)
    {
        fin >> ignore;
    }

    int rt_priority;
    fin >> rt_priority;

    info->priority = rt_priority;
}