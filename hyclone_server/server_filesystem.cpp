#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "haiku_errors.h"
#include "haiku_fcntl.h"
#include "haiku_fs_attr.h"
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
    vfsService.RegisterDevice(std::make_shared<SystemfsDevice>("/boot", std::filesystem::path(gHaikuPrefix) / "boot"));
    vfsService.RegisterDevice(std::make_shared<PackagefsDevice>("/boot/system", std::filesystem::path(gHaikuPrefix) / "boot/system"));
    vfsService.RegisterDevice(std::make_shared<SystemfsDevice>());

    vfsService.RegisterBuiltinFilesystem("packagefs", PackagefsDevice::Mount);
    vfsService.RegisterBuiltinFilesystem("systemfs", SystemfsDevice::Mount);
    vfsService.RegisterBuiltinFilesystem("bindfs", BindfsDevice::Mount);

    if (!server_setup_prefix())
    {
        std::cerr << "failed to setup system files." << std::endl;
    }

    return true;
}

intptr_t server_hserver_call_read_fs_info(hserver_context& context, int deviceId, void* info)
{
    std::shared_ptr<VfsDevice> device;

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();

        auto device = vfsService.GetDevice(deviceId).lock();
    }

    if (!device)
    {
        return B_BAD_VALUE;
    }

    auto procLock = context.process->Lock();

    if (context.process->WriteMemory(info, &device->GetInfo(), sizeof(haiku_fs_info)) != sizeof(haiku_fs_info))
    {
        return B_BAD_ADDRESS;
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

intptr_t server_hserver_call_mount(hserver_context& context, void* userPathAndSize, void* userDeviceAndSize,
    void* userFsNameAndSize, unsigned int flags, const char* userArgs, size_t userArgsSize)
{
    std::filesystem::path path;
    std::filesystem::path device;
    std::string fsName;
    std::string args;

    {
        auto lock = context.process->Lock();
        std::pair<const char*, size_t> pathAndSize;
        std::pair<const char*, size_t> deviceAndSize;
        std::pair<const char*, size_t> fsNameAndSize;
        std::string deviceString;

        if (context.process->ReadMemory(userPathAndSize, &pathAndSize, sizeof(pathAndSize)) != sizeof(pathAndSize))
        {
            return B_BAD_ADDRESS;
        }

        status_t status = context.process->ReadDirFd(HAIKU_AT_FDCWD, pathAndSize.first, pathAndSize.second, true, path);
        if (status != B_OK)
        {
            return status;
        }

        if (context.process->ReadMemory(userDeviceAndSize, &deviceAndSize, sizeof(deviceAndSize)) != sizeof(deviceAndSize))
        {
            return B_BAD_ADDRESS;
        }

        deviceString.resize(deviceAndSize.second);
        if (context.process->ReadMemory((void*)deviceAndSize.first, deviceString.data(), deviceAndSize.second) != deviceAndSize.second)
        {
            return B_BAD_ADDRESS;
        }
        device = deviceString;

        if (context.process->ReadMemory(userFsNameAndSize, &fsNameAndSize, sizeof(fsNameAndSize)) != sizeof(fsNameAndSize))
        {
            return B_BAD_ADDRESS;
        }

        fsName.resize(fsNameAndSize.second);
        if (context.process->ReadMemory((void*)fsNameAndSize.first, fsName.data(), fsNameAndSize.second) != fsNameAndSize.second)
        {
            return B_BAD_ADDRESS;
        }

        args.resize(userArgsSize);
        if (context.process->ReadMemory((void*)userArgs, args.data(), userArgsSize) != userArgsSize)
        {
            return B_BAD_ADDRESS;
        }
    }

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();

        return vfsService.Mount(path, device, fsName, flags, args);
    }
}

intptr_t server_hserver_call_unmount(hserver_context& context, const char* userPath, size_t userPathSize, unsigned int flags)
{
    std::filesystem::path path;

    {
        auto lock = context.process->Lock();

        status_t status = context.process->ReadDirFd(HAIKU_AT_FDCWD, userPath, userPathSize, true, path);
        if (status != B_OK)
        {
            return status;
        }
    }

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();

        return vfsService.Unmount(path, flags);
    }
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

        if (context.process->ReadMemory((void*)userStatBuf, &fullStat, sizeof(haiku_stat)) != sizeof(haiku_stat))
        {
            return B_BAD_ADDRESS;
        }
    }

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();
        status = vfsService.WriteStat(requestPath, fullStat, statMask, traverseSymlink);
    }

    if (status != B_OK)
    {
        return status;
    }

    return B_OK;
}

intptr_t server_hserver_call_stat_attr(hserver_context& context, int fd, const char* userName, size_t userNameSize,
    void* userAttrInfo)
{
    std::string name(userNameSize, '\0');
    haiku_attr_info info;
    std::filesystem::path requestPath;

    {
        auto lock = context.process->Lock();

        if (context.process->ReadMemory((void*)userName, name.data(), userNameSize) != userNameSize)
        {
            return B_BAD_ADDRESS;
        }

        if (context.process->ReadMemory((void*)userAttrInfo, &info, sizeof(haiku_attr_info)) != sizeof(haiku_attr_info))
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
        status = vfsService.StatAttr(requestPath, name, info);
    }

    if (status != B_OK)
    {
        return status;
    }

    {
        auto lock = context.process->Lock();

        if (context.process->WriteMemory((void*)userAttrInfo, &info, sizeof(haiku_attr_info)) != sizeof(haiku_attr_info))
        {
            return B_BAD_ADDRESS;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_transform_dirent(hserver_context& context, int fd, void* userEntry, size_t userEntrySize)
{
    std::vector<char> buffer(userEntrySize);
    haiku_dirent* entry = (haiku_dirent*)buffer.data();

    auto lock = context.process->Lock();

    if (context.process->ReadMemory(userEntry, entry, userEntrySize) != userEntrySize)
    {
        return B_BAD_ADDRESS;
    }

    if (!context.process->IsValidFd(fd))
    {
        return HAIKU_POSIX_EBADF;
    }

    const auto& requestPath = context.process->GetFd(fd);

    lock.unlock();

    status_t status;

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();
        status = vfsService.TransformDirent(requestPath, *entry);
    }

    if (status < 0)
    {
        return status;
    }

    {
        auto lock = context.process->Lock();

        if (context.process->WriteMemory(userEntry, entry, status) != status)
        {
            return B_BAD_ADDRESS;
        }
    }

    return status;
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

    if (status != B_OK && status != B_ENTRY_NOT_FOUND)
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

    return status;
}

intptr_t server_hserver_call_get_attr_path(hserver_context& context, int fd, void* userPathAndSize, void* userNameAndSize,
    unsigned int type, int openMode, void* userHostPathAndSize)
{
    std::string name;
    std::filesystem::path requestPath;
    status_t status;

    // bool traverseSymlink = !(openMode & HAIKU_O_NOTRAVERSE);
    // It seems that Haiku never traverses symlinks when opening an attribute.
    bool traverseSymlink = false;
    bool createNew = openMode & HAIKU_O_CREAT;

    {
        auto lock = context.process->Lock();

        std::pair<const char*, size_t> pathAndSize;
        std::pair<const char*, size_t> nameAndSize;

        if (context.process->ReadMemory(userPathAndSize, &pathAndSize, sizeof(pathAndSize)) != sizeof(pathAndSize))
        {
            return B_BAD_ADDRESS;
        }

        status = context.process->ReadDirFd(fd, pathAndSize.first, pathAndSize.second, traverseSymlink, requestPath);

        if (status != B_OK)
        {
            return status;
        }

        if (context.process->ReadMemory(userNameAndSize, &nameAndSize, sizeof(nameAndSize)) != sizeof(nameAndSize))
        {
            return B_BAD_ADDRESS;
        }

        name.resize(nameAndSize.second);

        if (context.process->ReadMemory((void*)nameAndSize.first, name.data(), nameAndSize.second) != nameAndSize.second)
        {
            return B_BAD_ADDRESS;
        }
    }

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();
        status = vfsService.GetAttrPath(requestPath, name, type, createNew, traverseSymlink);

        if (status != B_OK)
        {
            return status;
        }

        status = vfsService.GetPath(requestPath, false);

        if (status != B_OK && status != B_ENTRY_NOT_FOUND)
        {
            return status;
        }

        if (status == B_ENTRY_NOT_FOUND && !createNew)
        {
            return status;
        }

        status = 0;
    }

    std::string resultString = requestPath.string();

    {
        auto lock = context.process->Lock();

        std::pair<char*, size_t> hostPathAndSize;

        if (context.process->ReadMemory(userHostPathAndSize, &hostPathAndSize, sizeof(hostPathAndSize)) != sizeof(hostPathAndSize))
        {
            return B_BAD_ADDRESS;
        }

        if (resultString.size() >= hostPathAndSize.second)
        {
            return B_NAME_TOO_LONG;
        }

        size_t writeSize = resultString.size() + 1;

        if (context.process->WriteMemory(hostPathAndSize.first, resultString.data(), writeSize) != writeSize)
        {
            return B_BAD_ADDRESS;
        }
    }

    return status;
}

intptr_t server_hserver_call_read_attr(hserver_context& context, int fd, const char* userName, size_t userNameSize,
    size_t pos, void* userBuffer, size_t userBufferSize)
{
    std::string name;
    std::filesystem::path requestPath;
    intptr_t status;

    {
        auto lock = context.process->Lock();

        if (!context.process->IsValidFd(fd))
        {
            return HAIKU_POSIX_EBADF;
        }

        requestPath = context.process->GetFd(fd);

        name.resize(userNameSize);

        if (context.process->ReadMemory((void*)userName, name.data(), userNameSize) != userNameSize)
        {
            return B_BAD_ADDRESS;
        }
    }

    std::vector<char> buffer(userBufferSize);

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();
        status = vfsService.ReadAttr(requestPath, name, pos, buffer.data(), userBufferSize);
    }

    if (status < 0)
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

    return status;
}

intptr_t server_hserver_call_write_attr(hserver_context& context, int fd, void* userNameAndSize, unsigned int type,
    size_t pos, const void* userBuffer, size_t userBufferSize)
{
    std::string name;
    std::filesystem::path requestPath;
    std::vector<char> buffer;

    {
        auto lock = context.process->Lock();

        if (!context.process->IsValidFd(fd))
        {
            return HAIKU_POSIX_EBADF;
        }

        requestPath = context.process->GetFd(fd);

        std::pair<const char*, size_t> nameAndSize;
        if (context.process->ReadMemory(userNameAndSize, &nameAndSize, sizeof(nameAndSize)) != sizeof(nameAndSize))
        {
            return B_BAD_ADDRESS;
        }

        name.resize(nameAndSize.second);

        if (context.process->ReadMemory((void*)nameAndSize.first, name.data(), nameAndSize.second) != nameAndSize.second)
        {
            return B_BAD_ADDRESS;
        }

        buffer.resize(userBufferSize);

        if (context.process->ReadMemory((void*)userBuffer, buffer.data(), userBufferSize) != userBufferSize)
        {
            return B_BAD_ADDRESS;
        }
    }

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();
        return vfsService.WriteAttr(requestPath, name, type, pos, buffer.data(), userBufferSize);
    }
}

intptr_t server_hserver_call_remove_attr(hserver_context& context, int fd, const char* userName, size_t userNameSize)
{
    std::string name;
    std::filesystem::path requestPath;

    {
        auto lock = context.process->Lock();

        if (!context.process->IsValidFd(fd))
        {
            return HAIKU_POSIX_EBADF;
        }

        requestPath = context.process->GetFd(fd);

        name.resize(userNameSize);

        if (context.process->ReadMemory((void*)userName, name.data(), userNameSize) != userNameSize)
        {
            return B_BAD_ADDRESS;
        }
    }

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();
        return vfsService.RemoveAttr(requestPath, name);
    }
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
        status = vfsService.Ioctl(requestPath, op, userBuffer, buffer.data(), userBufferSize);
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

    return B_OK;
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
