#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "haiku_errors.h"
#include "haiku_fcntl.h"
#include "haiku_fs_info.h"
#include "process.h"
#include "server_filesystem.h"
#include "server_prefix.h"
#include "server_servercalls.h"
#include "system.h"

bool server_setup_filesystem()
{
    if (gHaikuPrefix.empty())
    {
        std::cerr << "HPREFIX not set." << std::endl;
        return false;
    }

    auto& system = System::GetInstance();
    auto& vfsService = system.GetVfsService();
    auto lock = system.Lock();

    haiku_fs_info info;

    // Create mount points
    std::filesystem::create_directories(gHaikuPrefix);
    std::filesystem::create_directories(std::filesystem::path(gHaikuPrefix) / "dev");
    std::filesystem::create_directories(std::filesystem::path(gHaikuPrefix) / "boot" / "system");

    vfsService.RegisterDevice(std::make_shared<RootfsDevice>(gHaikuPrefix));
    vfsService.RegisterDevice(std::make_shared<DevfsDevice>());
    vfsService.RegisterDevice(std::make_shared<PackagefsDevice>("/boot", std::filesystem::path(gHaikuPrefix) / "boot"));
    vfsService.RegisterDevice(std::make_shared<SystemfsDevice>());

    if (!server_setup_settings())
    {
        std::cerr << "failed to setup system settings." << std::endl;
    }

    return true;
}

intptr_t server_hserver_call_read_fs_info(hserver_context& context, int deviceId, void* info)
{
    auto& system = System::GetInstance();

    {
        auto& vfsService = system.GetVfsService();
        auto lock = vfsService.Lock();

        auto device = vfsService.GetDevice(deviceId).lock();

        if (!device)
        {
            return B_BAD_VALUE;
        }

        auto procLock = context.process->Lock();

        if (context.process->WriteMemory(info, &device->GetInfo(), sizeof(haiku_fs_info)) != sizeof(haiku_fs_info))
        {
            return B_BAD_ADDRESS;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_next_device(hserver_context& context, int* userCookie)
{
    int cookie;

    {
        auto lock = context.process->Lock();

        if (context.process->ReadMemory(userCookie, &cookie, sizeof(int)) != sizeof(int))
        {
            return B_BAD_ADDRESS;
        }
    }

    if (cookie == -1)
    {
        return B_BAD_VALUE;
    }

    int id = -1;

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();

        auto device = vfsService.GetDevice(cookie).lock();

        while (!device)
        {
            cookie = vfsService.NextDeviceId(cookie);

            if (cookie == -1)
            {
                break;
            }

            device = vfsService.GetDevice(cookie).lock();
        }

        if (!device)
        {
            return B_BAD_VALUE;
        }

        id = device->GetId();
        cookie = vfsService.NextDeviceId(cookie);
    }

    {
        auto lock = context.process->Lock();

        if (context.process->WriteMemory(userCookie, &cookie, sizeof(int)) != sizeof(int))
        {
            return B_BAD_ADDRESS;
        }
    }

    return id;
}

intptr_t server_hserver_call_read_stat(hserver_context& context, int fd, const char* userPath, size_t userPathSize,
    bool traverseSymlink, void* userStatBuf, size_t userStatSize)
{
    haiku_stat fullStat;

    std::filesystem::path requestPath;

    status_t status;

    {
        auto lock = context.process->Lock();
        status = context.process->ReadDirFd(fd, userPath, userPathSize, traverseSymlink, requestPath);
        if (status != B_OK)
        {
            return status;
        }
    }

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();
        status = vfsService.ReadStat(requestPath, fullStat, traverseSymlink);
    }

    if (status != B_OK)
    {
        return status;
    }

    {
        auto lock = context.process->Lock();

        if (context.process->WriteMemory(userStatBuf, &fullStat, userStatSize) != userStatSize)
        {
            return B_BAD_ADDRESS;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_write_stat(hserver_context& context, int fd, const char* userPath, size_t userPathSize,
    bool traverseSymlink, const void* userStatBuf, int statMask)
{
    haiku_stat fullStat;
    std::filesystem::path requestPath;

    status_t status;

    {
        auto lock = context.process->Lock();
        status = context.process->ReadDirFd(fd, userPath, userPathSize, traverseSymlink, requestPath);
        if (status != B_OK)
        {
            return status;
        }
    }

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();
        status = vfsService.WriteStat(requestPath, fullStat, traverseSymlink);
    }

    if (status != B_OK)
    {
        return status;
    }

    {
        auto lock = context.process->Lock();

        if (context.process->WriteMemory((void*)userStatBuf, &fullStat, sizeof(haiku_stat)) != sizeof(haiku_stat))
        {
            return B_BAD_ADDRESS;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_vchroot_expandat(hserver_context& context, int fd, const char* userPath, size_t userPathSize,
    bool traverseSymlink, char* userBuffer, size_t userBufferSize)
{
    std::filesystem::path requestPath;

    status_t status;

    {
        auto lock = context.process->Lock();
        status = context.process->ReadDirFd(fd, userPath, userPathSize, traverseSymlink, requestPath);
        if (status != B_OK)
        {
            return status;
        }
    }

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();
        status = vfsService.GetPath(requestPath, traverseSymlink);
    }

    if (status != B_OK)
    {
        return status;
    }

    std::string resultString = requestPath.string();

    if (resultString.size() >= userBufferSize)
    {
        return B_NAME_TOO_LONG;
    }

    {
        auto lock = context.process->Lock();

        size_t writeSize = resultString.size() + 1;

        if (context.process->WriteMemory(userBuffer, resultString.c_str(), writeSize) != writeSize)
        {
            return B_BAD_ADDRESS;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_ioctl(hserver_context& context, int fd, unsigned int op, void* userBuffer, size_t userBufferSize)
{
    std::filesystem::path requestPath;
    std::vector<char> buffer(userBufferSize);

    {
        auto lock = context.process->Lock();

        if (context.process->ReadMemory(userBuffer, buffer.data(), userBufferSize) != userBufferSize)
        {
            return B_BAD_ADDRESS;
        }

        if (!context.process->IsValidFd(fd))
        {
            return HAIKU_POSIX_EBADF;
        }

        requestPath = context.process->GetFd(fd);
    }

    status_t status;

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();
        status = vfsService.Ioctl(requestPath, op, buffer.data(), userBufferSize);
    }

    if (status != B_OK)
    {
        return status;
    }

    {
        auto lock = context.process->Lock();

        if (context.process->WriteMemory(userBuffer, buffer.data(), userBufferSize) != userBufferSize)
        {
            return B_BAD_ADDRESS;
        }
    }
}

// Sample filesystem info of a Haiku machine.
// ~/Desktop> ./statfs
// dev: 1
// root: 1
// flags: 0
// block_size: 0
// io_size: 0
// total_blocks: 0
// free_blocks: 0
// total_nodes: 0
// free_nodes: 0
// device_name:
// volume_name:
// fsh_name: rootfs
// ==============
// dev: 2
// root: 0
// flags: 0
// block_size: 0
// io_size: 0
// total_blocks: 0
// free_blocks: 0
// total_nodes: 0
// free_nodes: 0
// device_name:
// volume_name:
// fsh_name: devfs
// ==============
// dev: 3
// root: 524288
// flags: 4653060
// block_size: 2048
// io_size: 65536
// total_blocks: 16776704
// free_blocks: 7638557
// total_nodes: 0
// free_nodes: 0
// device_name: /dev/disk/nvme/0/0
// volume_name: Haiku_SSD
// fsh_name: bfs
// ==============
// dev: 4
// root: 1
// flags: 2555909
// block_size: 4096
// io_size: 65536
// total_blocks: 1
// free_blocks: 1
// total_nodes: 0
// free_nodes: 0
// device_name:
// volume_name: system
// fsh_name: packagefs
// ==============
// dev: 5
// root: 1
// flags: 2555909
// block_size: 4096
// io_size: 65536
// total_blocks: 1
// free_blocks: 1
// total_nodes: 0
// free_nodes: 0
// device_name:
// volume_name: config
// fsh_name: packagefs
// ==============
// dev: 6
// root: 524288
// flags: 4653060
// block_size: 2048
// io_size: 65536
// total_blocks: 12582912
// free_blocks: 8767256
// total_nodes: 0
// free_nodes: 0
// device_name: /dev/disk/ata/0/slave/1
// volume_name: Data
// fsh_name: bfs
// ==============
