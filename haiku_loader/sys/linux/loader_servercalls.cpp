#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <pthread.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "haiku_errors.h"
#include "loader_servercalls.h"
#include "loader_vchroot.h"
#include "servercalls.h"

class ServerConnection
{
private:
    int _socket;
    struct sockaddr_un _addr;

public:
    ServerConnection();
    ~ServerConnection();

    bool Connect(bool forceReconnect = false);
    void Disconnect();

    bool IsConnected() const { return _socket != -1; }
    bool Send(const void* data, size_t size);
    bool Receive(void* data, size_t size);
};

ServerConnection::ServerConnection()
{
    _socket = -1;
    memset(&_addr, 0, sizeof(struct sockaddr_un));
    _addr.sun_family = AF_UNIX;
    auto hycloneSocketPath = std::filesystem::path(gHaikuPrefix) / HYCLONE_SOCKET_NAME;
    strncpy(_addr.sun_path, hycloneSocketPath.c_str(), sizeof(_addr.sun_path) - 1);
}

ServerConnection::~ServerConnection()
{
    Disconnect();
}

bool ServerConnection::Connect(bool forceReconnect)
{
    if (IsConnected())
    {
        if (!forceReconnect)
            return true;
        close(_socket);
        _socket = -1;
    }

    _socket = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (_socket < 0)
    {
        _socket = -1;
        return false;
    }

    int ret = connect(_socket, (const struct sockaddr *)&_addr,
                   sizeof(struct sockaddr_un));
    if (ret == -1)
    {
        close(_socket);
        _socket = -1;
        return false;
    }

    int maxFd = sysconf(_SC_OPEN_MAX) - 1;
    while (maxFd >= 0 && fcntl(maxFd, F_GETFD) != -1)
    {
        --maxFd;
    }

    if (maxFd >= 0)
    {
        dup2(_socket, maxFd);
        close(_socket);
        _socket = maxFd;
    }

    intptr_t args[HYCLONE_SERVERCALL_MAX_ARGS + 1] =
        { SERVERCALL_ID_connect, getpid(), syscall(SYS_gettid), 0, 0, 0, 0 };
    intptr_t returnCode = -1;

    if (!Send(args, sizeof(args))) goto fail;
    if (!Receive(&returnCode, sizeof(returnCode))) goto fail;
    if (returnCode != 0) goto fail;

    return true;
fail:
    close(_socket);
    _socket = -1;
    return false;
}

void ServerConnection::Disconnect()
{
    if (IsConnected())
    {
        intptr_t args[HYCLONE_SERVERCALL_MAX_ARGS + 1] = { SERVERCALL_ID_disconnect, 0, 0, 0, 0, 0, 0 };
        intptr_t returnCode = -1;
        Send(args, sizeof(args));
        Receive(&returnCode, sizeof(returnCode));
        close(_socket);
        _socket = -1;
    }
}

bool ServerConnection::Send(const void* data, size_t size)
{
    size_t sent = 0;
    while (sent < size)
    {
        ssize_t ret = write(_socket, (const char*)data + sent, size - sent);
        if (ret == -1)
        {
            return false;
        }
        sent += ret;
    }
    return true;
}

bool ServerConnection::Receive(void* data, size_t size)
{
    size_t received = 0;
    while (received < size)
    {
        ssize_t ret = read(_socket, (char*)data + received, size - received);
        if (ret == -1)
        {
            return false;
        }
        received += ret;
    }
    return true;
}

static intptr_t loader_hserver_call(servercall_id id, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6);

#define HYCLONE_SERVERCALL0(name) \
    intptr_t loader_hserver_call_##name() \
    { \
        return loader_hserver_call(SERVERCALL_ID_##name, 0, 0, 0, 0, 0, 0); \
    }

#define HYCLONE_SERVERCALL1(name, arg1) \
    intptr_t loader_hserver_call_##name(arg1 a1) \
    { \
        return loader_hserver_call(SERVERCALL_ID_##name, (intptr_t)a1, 0, 0, 0, 0, 0); \
    }
#define HYCLONE_SERVERCALL2(name, arg1, arg2) \
    intptr_t loader_hserver_call_##name(arg1 a1, arg2 a2) \
    { \
        return loader_hserver_call(SERVERCALL_ID_##name, (intptr_t)a1, (intptr_t)a2, 0, 0, 0, 0); \
    }
#define HYCLONE_SERVERCALL3(name, arg1, arg2, arg3) \
    intptr_t loader_hserver_call_##name(arg1 a1, arg2 a2, arg3 a3) \
    { \
        return loader_hserver_call(SERVERCALL_ID_##name, (intptr_t)a1, (intptr_t)a2, (intptr_t)a3, 0, 0, 0); \
    }
#define HYCLONE_SERVERCALL4(name, arg1, arg2, arg3, arg4) \
    intptr_t loader_hserver_call_##name(arg1 a1, arg2 a2, arg3 a3, arg4 a4) \
    { \
        return loader_hserver_call(SERVERCALL_ID_##name, (intptr_t)a1, (intptr_t)a2, (intptr_t)a3, (intptr_t)a4, 0, 0); \
    }
#define HYCLONE_SERVERCALL5(name, arg1, arg2, arg3, arg4, arg5) \
    intptr_t loader_hserver_call_##name(arg1 a1, arg2 a2, arg3 a3, arg4 a4, arg5 a5) \
    { \
        return loader_hserver_call(SERVERCALL_ID_##name, (intptr_t)a1, (intptr_t)a2, (intptr_t)a3, (intptr_t)a4, (intptr_t)a5, 0); \
    }
#define HYCLONE_SERVERCALL6(name, arg1, arg2, arg3, arg4, arg5, arg6) \
    intptr_t loader_hserver_call_##name(arg1 a1, arg2 a2, arg3 a3, arg4 a4, arg5 a5, arg6 a6) \
    { \
        return loader_hserver_call(SERVERCALL_ID_##name, (intptr_t)a1, (intptr_t)a2, (intptr_t)a3, (intptr_t)a4, (intptr_t)a5, (intptr_t)a6); \
    }

#include "servercall_defs.h"

#undef HYCLONE_SERVERCALL0
#undef HYCLONE_SERVERCALL1
#undef HYCLONE_SERVERCALL2
#undef HYCLONE_SERVERCALL3
#undef HYCLONE_SERVERCALL4
#undef HYCLONE_SERVERCALL5
#undef HYCLONE_SERVERCALL6

// One connection per THREAD.
thread_local ServerConnection gServerConnection;
thread_local bool gServerThreadInit = false;

static bool loader_hserver_thread_init();

intptr_t loader_hserver_call(servercall_id id, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6)
{
    if (!gServerThreadInit)
    {
        gServerThreadInit = loader_hserver_thread_init();
    }

    intptr_t args[HYCLONE_SERVERCALL_MAX_ARGS + 1] = { id, a1, a2, a3, a4, a5, a6 };
    if (!gServerConnection.Connect())
    {
        return HAIKU_POSIX_ENOSYS;
    }

    if (!gServerConnection.Send(args, sizeof(args)))
    {
        gServerConnection.Disconnect();
        return HAIKU_POSIX_ENOSYS;
    }

    intptr_t result;
    if (!gServerConnection.Receive(&result, sizeof(result)))
    {
        gServerConnection.Disconnect();
        return HAIKU_POSIX_ENOSYS;
    }

    return result;
}

bool loader_hserver_child_atfork()
{
    if (!gServerConnection.Connect(/* forceReconnect: */true))
    {
        return false;
    }
    return loader_hserver_call(SERVERCALL_ID_wait_for_fork_unlock, 0, 0, 0, 0, 0, 0) == 0;
}

static bool loader_hserver_thread_init()
{
    return gServerConnection.Connect();
}

bool loader_init_servercalls()
{
    bool success = gServerConnection.Connect();
    if (success)
    {
        return true;
    }

    std::cerr << "Attempting to launch HyClone server..." << std::endl;

    pid_t pid = fork();
    if (pid == 0)
    {
        std::filesystem::path server_path = std::filesystem::canonical("/proc/self/exe").parent_path() / "hyclone_server";

        const char* argv[] = { server_path.c_str(), NULL };
        execv(argv[0], (char* const*)argv);
        _exit(1);
    }

    if (pid == -1)
    {
        return false;
    }

    if (waitpid(pid, NULL, 0) == -1)
    {
        return false;
    }

    // Here, the server should have daemonized and is listening to the socket.

    if (!gServerConnection.Connect())
    {
        return false;
    }

    return true;
}
