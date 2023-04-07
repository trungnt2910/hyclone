#include "extended_signal.h"
#include <signal.h>

#include <filesystem>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "BeDefs.h"
#include "haiku_area.h"
#include "loader_requests.h"
#include "loader_runtime.h"
#include "loader_vchroot.h"
#include "requests.h"
#include "servercalls.h"

static void loader_handle_request(int sig, siginfo_t* info, void* context);
static intptr_t loader_handle_request(const TransferAreaRequestArgs& request);

bool loader_init_requests()
{
    struct sigaction action;
    action.sa_sigaction = loader_handle_request;
    action.sa_flags = SA_SIGINFO | SA_RESTART;
    sigfillset(&action.sa_mask);
    if (sigaction(SIGREQUEST, &action, NULL) != 0)
    {
        return false;
    }

    return true;
}

class RequestContext
{
private:
    int _socket;
    char* _args = NULL;

    bool _Send(const void* data, size_t size);
    bool _Receive(void* data, size_t size);
public:
    RequestContext();
    ~RequestContext();

    template <typename T>
    T& Arguments() { return *(T*)_args; }

    template <typename T>
    const T& Arguments() const { return *(const T*)_args; }

    request_id RequestId() const { return (request_id)Arguments<RequestArgs>().id; }
    int Socket() const { return _socket; }

    bool Reply(intptr_t result);
};

RequestContext::RequestContext()
{
    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    auto hycloneSocketPath = std::filesystem::path(gHaikuPrefix) / HYCLONE_SOCKET_NAME;
    strncpy(addr.sun_path, hycloneSocketPath.c_str(), sizeof(addr.sun_path) - 1);

    _socket = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

    if (_socket < 0)
    {
        throw std::system_error(errno, std::generic_category(), "failed to create socket");
    }

    if (connect(_socket, (const struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        throw std::system_error(errno, std::generic_category(), "failed to connect to hyclone");
    }

    intptr_t serverArgs[HYCLONE_SERVERCALL_MAX_ARGS + 1] =
        { SERVERCALL_ID_request_ack, getpid(), gettid() };
    intptr_t returnCode = -1;

    if (!_Send(serverArgs, sizeof(serverArgs)))
    {
        throw std::system_error(errno, std::generic_category(), "failed to send request ack");
    }

    if (!_Receive(&returnCode, sizeof(returnCode)))
    {
        throw std::system_error(errno, std::generic_category(), "failed to receive request ack");
    }

    if (returnCode < 0)
    {
        throw std::runtime_error("failed to receive request ack");
    }

    _args = new char[returnCode];

    serverArgs[0] = SERVERCALL_ID_request_read;
    serverArgs[1] = (intptr_t)_args;

    if (!_Send(serverArgs, sizeof(serverArgs)))
    {
        throw std::system_error(errno, std::generic_category(), "failed to send request read");
    }

    if (!_Receive(&returnCode, sizeof(returnCode)))
    {
        throw std::system_error(errno, std::generic_category(), "failed to receive request read");
    }

    if (returnCode < 0)
    {
        throw std::runtime_error("failed to receive request read");
    }
}

RequestContext::~RequestContext()
{
    if (_args)
    {
        delete[] _args;
    }

    Reply(HAIKU_POSIX_ENOSYS);

    if (_socket != -1)
    {
        intptr_t serverArgs[HYCLONE_SERVERCALL_MAX_ARGS + 1] =
            { SERVERCALL_ID_disconnect };

        _Send(serverArgs, sizeof(serverArgs));
        close(_socket);
    }
}

bool RequestContext::Reply(intptr_t result)
{
    if (_socket == -1)
    {
        return false;
    }

    intptr_t serverArgs[HYCLONE_SERVERCALL_MAX_ARGS + 1] =
        { SERVERCALL_ID_request_reply, result };
    intptr_t returnCode = -1;

    if (!_Send(serverArgs, sizeof(serverArgs)))
    {
        return false;
    }

    if (!_Receive(&returnCode, sizeof(returnCode)))
    {
        return false;
    }

    serverArgs[0] = SERVERCALL_ID_disconnect;
    _Send(serverArgs, sizeof(serverArgs));

    close(_socket);
    _socket = -1;

    return true;
}

bool RequestContext::_Send(const void* data, size_t size)
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

bool RequestContext::_Receive(void* data, size_t size)
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

struct ServerConnection
{
    int _socket;
};
extern thread_local ServerConnection gServerConnection;

static void loader_handle_request(int sig, siginfo_t* info, void* context)
{
    RequestContext requestContext;

    int oldSocket = gServerConnection._socket;
    gServerConnection._socket = requestContext.Socket();

    switch (requestContext.RequestId())
    {
        case REQUEST_ID_transfer_area:
            requestContext.Reply(loader_handle_request(requestContext.Arguments<TransferAreaRequestArgs>()));
        break;
    }

    gServerConnection._socket = oldSocket;
}

extern intptr_t loader_hserver_call_get_area_info(int id, void* info);

static intptr_t loader_handle_request(const TransferAreaRequestArgs& request)
{
    using kern_clone_area_t = area_id (*)(const char *, void **, uint32, uint32, area_id);
    static kern_clone_area_t kern_clone_area = (kern_clone_area_t)loader_runtime_symbol("_kern_clone_area");

    haiku_area_info info;
    status_t status = loader_hserver_call_get_area_info(request.baseArea, &info);

    if (status != B_OK)
    {
        return status;
    }

    void* address = request.address;
    return kern_clone_area(info.name, &address, request.addressSpec, info.protection, request.transferredArea);
}