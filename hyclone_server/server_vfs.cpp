#include <cstring>

#include "haiku_errors.h"
#include "server_vfs.h"

VfsDevice::VfsDevice(const std::filesystem::path& root, const haiku_fs_info& info)
    : _root(root), _info(info)
{
}

VfsService::VfsService()
{
    // Take up the device ID 0.
    _devices.Add(std::shared_ptr<VfsDevice>());
}

size_t VfsService::RegisterEntryRef(const EntryRef& ref, const std::string& path)
{
    std::string tmpPath = path;
    tmpPath.shrink_to_fit();
    return RegisterEntryRef(ref, std::move(tmpPath));
}

size_t VfsService::RegisterEntryRef(const EntryRef& ref, std::string&& path)
{
    // TODO: Limit the total number of entryRefs stored.
    _entryRefs[ref] = path;
    return _entryRefs.size();
}

bool VfsService::GetEntryRef(const EntryRef& ref, std::string& path) const
{
    auto it = _entryRefs.find(ref);
    if (it != _entryRefs.end())
    {
        path.resize(it->second.size());
        memcpy(path.data(), it->second.c_str(), it->second.size() + 1);
        return true;
    }
    return false;
}

size_t VfsService::RegisterDevice(const std::shared_ptr<VfsDevice>& device)
{
    const auto& path = device->GetRoot();

    device->GetInfo().dev = _devices.Add(device);
    _deviceMounts[path] = device;

    return _devices.Size();
}

std::weak_ptr<VfsDevice> VfsService::GetDevice(int id)
{
    if (_devices.IsValidId(id))
    {
        return _devices.Get(id);
    }
    return std::weak_ptr<VfsDevice>();
}

std::weak_ptr<VfsDevice> VfsService::GetDevice(const std::filesystem::path& path)
{
    assert(path.is_absolute());
    assert(path.lexically_normal() == path);

    auto it = _deviceMounts.find(path);
    if (it != _deviceMounts.end())
    {
        return it->second;
    }
    return std::weak_ptr<VfsDevice>();
}

status_t VfsService::GetPath(std::filesystem::path& path, bool traverseLink)
{
    return _DoWork(path, traverseLink, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        return device->GetPath(currentPath, isSymlink);
    });
}

status_t VfsService::ReadStat(const std::filesystem::path& path, haiku_stat& stat, bool traverseLink)
{
    return _DoWork(path, traverseLink, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        return device->ReadStat(currentPath, stat, isSymlink);
    });
}

status_t VfsService::WriteStat(const std::filesystem::path& path, const haiku_stat& stat,
    int statMask, bool traverseLink)
{
    return _DoWork(path, traverseLink, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        return device->WriteStat(currentPath, stat, statMask, isSymlink);
    });
}

status_t VfsService::OpenDir(const std::filesystem::path& path, VfsDir& dir, bool traverseLink)
{
    status_t status = _DoWork(path, traverseLink, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        return device->OpenDir(currentPath, dir, isSymlink);
    });
    if (status == B_OK)
    {
        dir.cookie = (size_t)-3;
    }
    return status;
}

status_t VfsService::ReadDir(VfsDir& dir, haiku_dirent& dirent)
{
    auto device = dir.device.lock();
    if (!device)
    {
        return B_ENTRY_NOT_FOUND;
    }
    if (dir.cookie == (size_t)-3 || dir.cookie == (size_t)-2)
    {
        std::filesystem::path path;
        std::string name;
        if (dir.cookie == (size_t)-3)
        {
            path = dir.path;
            name = ".";
        }
        else
        {
            path = dir.path.parent_path();
            name = "..";
        }

        auto parentPath = path.parent_path();

        size_t maxFileNameLen = dirent.d_reclen - sizeof(haiku_dirent);

        if (name.size() >= maxFileNameLen)
        {
            return B_BUFFER_OVERFLOW;
        }

        haiku_stat stat;
        status_t status = ReadStat(path, stat, false);
        if (status != B_OK)
        {
            return status;
        }

        haiku_stat pstat;
        status = ReadStat(parentPath, pstat, false);
        if (status != B_OK)
        {
            return status;
        }

        dirent.d_dev = stat.st_dev;
        dirent.d_ino = stat.st_ino;
        dirent.d_pdev = pstat.st_dev;
        dirent.d_pino = pstat.st_ino;
        dirent.d_reclen = sizeof(haiku_dirent) + name.size() + 1;
        name.copy(dirent.d_name, name.size());
        dirent.d_name[name.size()] = '\0';

        if (dir.cookie == (size_t)-3)
        {
            dir.cookie = (size_t)-2;
        }
        else
        {
            dir.cookie = 0;
        }

        return B_OK;
    }
    else
    {
        return device->ReadDir(dir, dirent);
    }
}

status_t VfsService::RewindDir(VfsDir& dir)
{
    auto device = dir.device.lock();
    if (!device)
    {
        return B_ENTRY_NOT_FOUND;
    }
    status_t status = device->RewindDir(dir);
    if (status == B_OK)
    {
        dir.cookie = (size_t)-3;
    }
    return status;
}

status_t VfsService::_DoWork(std::filesystem::path& path, bool traverseLink, const callback_t& work)
{
    path = path.lexically_normal();
    bool isSymlink = traverseLink;

    do
    {
        std::filesystem::path drivePath = path;

        while (drivePath.has_parent_path() && !_deviceMounts.contains(drivePath))
        {
            drivePath = drivePath.parent_path();
        }

        if (_deviceMounts.contains(drivePath))
        {
            auto device = _deviceMounts[drivePath];
            status_t status = work(path, device, isSymlink);
            if (status != B_OK)
            {
                return status;
            }
        }
        else
        {
            return B_ENTRY_NOT_FOUND;
        }
    }
    while (isSymlink);

    return B_OK;
}

status_t VfsService::_DoWork(const std::filesystem::path& path, bool traverseLink, const callback_t& work)
{
    auto tmp = path;
    return _DoWork(tmp, traverseLink, work);
}