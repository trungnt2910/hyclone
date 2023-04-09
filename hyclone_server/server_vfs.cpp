#include <cstring>

#include "haiku_errors.h"
#include "server_native.h"
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

    haiku_stat stat;
    ReadStat(device->GetRoot(), stat, false);
    device->GetInfo().root = stat.st_ino;

    RegisterEntryRef(EntryRef(device->GetInfo().dev, device->GetInfo().root), path.string());

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

status_t VfsService::GetAttrPath(std::filesystem::path& path, const std::string& name, uint32 type,
    bool createNew, bool traverseLink)
{
    return _DoWork(path, traverseLink, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        return device->GetAttrPath(currentPath, name, type, createNew, isSymlink);
    });
}

status_t VfsService::RealPath(std::filesystem::path& path)
{
    auto tempPath = path;
    return _DoWork(tempPath, true, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        status_t status = device->GetPath(currentPath, isSymlink);
        if (isSymlink)
        {
            path = currentPath;
        }
        return status;
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

status_t VfsService::StatAttr(const std::filesystem::path& path, const std::string& name,
    haiku_attr_info& info)
{
    return _DoWork(path, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        return device->StatAttr(currentPath, name, info);
    });
}

status_t VfsService::TransformDirent(const std::filesystem::path& path, haiku_dirent& dirent)
{
    return _DoWork(path, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        return device->TransformDirent(path, dirent);
    });
}

haiku_ssize_t VfsService::ReadAttr(const std::filesystem::path& path, const std::string& name, size_t pos,
    void* buffer, size_t size)
{
    return _DoWork<haiku_ssize_t>(path, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        return device->ReadAttr(currentPath, name, pos, buffer, size);
    });
}

haiku_ssize_t VfsService::WriteAttr(const std::filesystem::path& path, const std::string& name,
    uint32 type, size_t pos, const void* buffer, size_t size)
{
    return _DoWork<haiku_ssize_t>(path, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        return device->WriteAttr(currentPath, name, type, pos, buffer, size);
    });
}

status_t VfsService::RemoveAttr(const std::filesystem::path& path, const std::string& name)
{
    return _DoWork(path, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        return device->RemoveAttr(currentPath, name);
    });
}

status_t VfsService::Ioctl(const std::filesystem::path& path, unsigned int cmd, void* addr, void* buffer, size_t size)
{
    return _DoWork(path, [&](std::filesystem::path& currentPath,
        const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
    {
        return device->Ioctl(path, cmd, addr, buffer, size);
    });
}