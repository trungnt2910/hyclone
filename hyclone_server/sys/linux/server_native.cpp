#include <cstddef>
#include <cstring>
#include <iostream>
// #include <sys/capability.h>
#include <sys/uio.h>
#include <signal.h>
#include <unistd.h>

#include "server_native.h"

// static bool ensure_ptrace_capability();

size_t server_read_process_memory(int pid, void* address, void* buffer, size_t size)
{
    // if (!ensure_ptrace_capability())
    // {
    //     std::cerr << "Cannot read process memory: Failed to gain ptrace capability." << std::endl;
    //     return (size_t)-1;
    // }

    struct iovec local_iov = { buffer, size };
    struct iovec remote_iov = { address, size };

    return (size_t)process_vm_readv((pid_t)pid, &local_iov, 1, &remote_iov, 1, 0);
}
size_t server_write_process_memory(int pid, void* address, const void* buffer, size_t size)
{
    // if (!ensure_ptrace_capability())
    // {
    //     std::cerr << "Cannot write process memory: Failed to gain ptrace capability." << std::endl;
    //     return (size_t)-1;
    // }

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

// static bool ensure_ptrace_capability()
// {
//     static bool already_succeeded = false;

//     if (already_succeeded)
//     {
//         return true;
//     }

//     cap_t caps = cap_get_proc();
//     cap_value_t cap_list[] = { CAP_SYS_PTRACE };
//     bool success = true;

//     if (caps == NULL)
//     {
//         return false;
//     }
//     if (cap_set_flag(caps, CAP_EFFECTIVE, sizeof(cap_list), cap_list, CAP_SET) == -1)
//     {
//         std::cerr << "Failed to set flag." << std::endl;
//         success = false;
//         goto fail;
//     }
//     if (cap_set_proc(caps) == -1)
//     {
//         std::cerr << "Failed to set caps for server process." << std::endl;
//         std::cerr << strerror(errno) << std::endl;
//         success = false;
//         goto fail;
//     }
//     already_succeeded = true;

// fail:
//     cap_free(caps);
//     return success;
// }