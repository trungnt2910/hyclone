#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "servercalls.h"
#include "server_prefix.h"
#include "server_servercalls.h"
#include "server_workers.h"
#include "system.h"

#include "server_main.h"

const int kBufferLength = (1 + HYCLONE_SERVERCALL_MAX_ARGS);
const int kBufferSize = sizeof(intptr_t) * kBufferLength;

int server_main(int argc, char **argv)
{
    struct sockaddr_un addr;
    int status;

    std::string hycloneSocketPath = std::filesystem::path(gHaikuPrefix) / HYCLONE_SOCKET_NAME;

    // daemon(0, 0);
    unlink(hycloneSocketPath.c_str());

    int listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_socket < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, hycloneSocketPath.c_str(), sizeof(addr.sun_path) - 1);
    status = bind(listen_socket, (const struct sockaddr *)&addr,
                  sizeof(struct sockaddr_un));
    if (status == -1)
    {
        perror("bind");
        return 1;
    }

    status = listen(listen_socket, 128);
    if (status == -1)
    {
        perror("listen");
        return 1;
    }

    std::vector<pollfd> pollfds;
    pollfds.push_back({listen_socket, POLLIN, 0});

    auto& system = System::GetInstance();
    {
        auto lock = system.Lock();
        // Register the master socket.
        system.RegisterConnection(listen_socket, 0, 0);
    }

    while (!system.IsShuttingDown())
    {
        status = poll(pollfds.data(), (nfds_t)pollfds.size(), -1);
        if (status == -1)
        {
            perror("poll");
            return 1;
        }

        size_t oldSize = pollfds.size();
        for (size_t i = 0; i < oldSize; ++i)
        {
            const auto &[fd, events, revents] = pollfds[i];
            if (fd == listen_socket)
            {
                if (revents & POLLIN)
                {
                    int data_socket = accept(listen_socket, NULL, NULL);
                    if (data_socket < 0)
                    {
                        perror("accept");
                        return 1;
                    }
                    std::cerr << "accepted connection" << std::endl;
                    pollfds.push_back({data_socket, POLLIN, 0});
                }
            }
            else if (revents & POLLIN)
            {
                int inputBytes = 0;
                if (ioctl(fd, FIONREAD, &inputBytes) == -1)
                {
                    continue;
                }
                // POLLIN set but no bytes available. This
                // probably means that the client has closed
                // the connection.
                if (inputBytes == 0)
                {
                    goto connection_close;
                }
                // Not enough data for a servercall.
                if (inputBytes < kBufferSize)
                {
                    continue;
                }

                std::unique_ptr<intptr_t[]> buffer(new intptr_t[kBufferLength]);
                ssize_t bytes_read = read(fd, buffer.get(), kBufferSize);
                if (bytes_read == -1 || bytes_read != kBufferSize)
                {
                    std::cerr << "read failed" << std::endl;
                }
                std::cerr << "received servercall: " + std::to_string(buffer[0]) << std::endl;

                const auto dispatch = [&](int fd, std::unique_ptr<intptr_t[]> buffer)
                {
                    intptr_t returnValue =
                        server_dispatch(fd,
                            buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);

                    write(fd, &returnValue, sizeof(returnValue));
                };

                if (buffer[0] == SERVERCALL_ID_disconnect)
                {
                    // Synchronously dispatch this call to
                    // quickly free the invalid fd.
                    dispatch(fd, std::move(buffer));
                    goto connection_close;
                }

                server_worker_run(dispatch, fd, std::move(buffer));
            }
            else if (revents & POLLHUP || revents & POLLERR)
            {
            connection_close:
                std::cerr << "Closing: " << fd << std::endl;
                close(fd);
                // For process that have already been gracefully disconnected,
                // the call should silently fail with a B_BAD_VALUE on server_dispatch
                // before reaching server_hserver_call_disconnect.
                server_dispatch(fd, SERVERCALL_ID_disconnect, 0, 0, 0, 0, 0, 0);

                std::swap(pollfds[i], pollfds.back());
                pollfds.pop_back();
                --oldSize;

                // This means that pollfds.back() is actually a valid entry,
                // as no new entry has been added. We need to iterate through it.
                if (oldSize == pollfds.size())
                {
                    --i;
                }

                for (const auto & open : pollfds)
                {
                    std::cerr << open.fd << " ";
                }
                std::cerr << std::endl;
            }
        }
    }

    return 0;
}