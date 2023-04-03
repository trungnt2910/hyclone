#include <cstring>

#include "haiku_errors.h"
#include "server_vfs.h"

VfsDevice::VfsDevice(const std::filesystem::path& root, const haiku_fs_info& info)
    : _root(root), _info(info)
{
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

    if (path.has_parent_path())
    {
        _mountPoints[path.parent_path()] = path.filename().string();
    }

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
    return _DoWork(path, traverseLink, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        return device->OpenDir(currentPath, dir, isSymlink);
    });
}

status_t VfsService::ReadDir(VfsDir& dir, haiku_dirent& dirent)
{
    auto device = dir.device.lock();
    if (!device)
    {
        return B_ENTRY_NOT_FOUND;
    }
    return device->ReadDir(dir, dirent);
}

status_t VfsService::RewindDir(VfsDir& dir)
{
    auto device = dir.device.lock();
    if (!device)
    {
        return B_ENTRY_NOT_FOUND;
    }
    return device->RewindDir(dir);
}

status_t VfsService::_DoWork(std::filesystem::path& path, bool traverseLink, const callback_t& work)
{
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