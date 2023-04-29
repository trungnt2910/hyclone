#include <cstring>

#include "haiku_errors.h"
#include "haiku_fs_volume.h"
#include "server_filesystem.h"
#include "server_native.h"
#include "server_nodemonitor.h"
#include "server_vfs.h"
#include "system.h"

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

size_t VfsService::UnregisterEntryRef(const EntryRef& ref)
{
    _entryRefs.erase(ref);
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

bool VfsService::SearchEntryRef(const std::string& path, EntryRef& ref) const
{
    for (const auto& [entryRef, entryPath] : _entryRefs)
    {
        if (entryPath == path)
        {
            ref = entryRef;
            return true;
        }
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

void VfsService::RegisterBuiltinFilesystem(const std::string& name, mounter_t mounter)
{
    _mounters[name] = mounter;
}

haiku_dev_t VfsService::Mount(const std::filesystem::path& path, const std::filesystem::path& device,
    const std::string& fsName, uint32 flags, const std::string& args)
{
    std::shared_ptr<VfsDevice> volume;

    auto realPath = path.lexically_normal();
    status_t status = RealPath(realPath);

    if (status != B_OK)
    {
        return status;
    }

    struct haiku_stat st;
    status = ReadStat(realPath, st, false);

    if (status != B_OK)
    {
        return status;
    }

    if (!HAIKU_S_ISDIR(st.st_mode))
    {
        return B_NOT_A_DIRECTORY;
    }

    status = B_DEVICE_NOT_FOUND;

    if (_mounters.contains(fsName))
    {
        status = _mounters[fsName](realPath, device, flags, args, volume);
    }
    else
    {
        // TODO: Load from external "drivers".
        // The VfsDevice class is a header-only class with only virtual
        // and inline functions. Therefore third-parties can inherit
        // this class and create ABI-compatible shared objects provided
        // that they use a compatible compiler.
    }

    if (status != B_OK)
    {
        return status;
    }

    RegisterDevice(volume);

    System::GetInstance().GetNodeMonitorService()
        .NotifyMount(volume->GetInfo().dev, st.st_dev, st.st_ino);

    return volume->GetInfo().dev;
}

status_t VfsService::Unmount(const std::filesystem::path& path, uint32 flags)
{
    auto realPath = path.lexically_normal();
    status_t status = RealPath(realPath);

    if (status != B_OK)
    {
        return status;
    }

    auto it = _deviceMounts.find(realPath);
    if (it != _deviceMounts.end())
    {
        // TODO: Check if the device is still in use.
        haiku_dev_t dev = it->second->GetInfo().dev;

        status = it->second->Cleanup();
        if (status != B_OK)
        {
            return status;
        }

        _devices.Remove(dev);
        _deviceReferences.erase(dev);
        for (auto& monitor : _monitors[dev])
        {
            it->second->RemoveMonitor(monitor);
        }
        _monitors.erase(dev);
        std::vector<decltype(_entryRefs)::iterator> toRemove;
        for (auto refIt = _entryRefs.begin(); refIt != _entryRefs.end(); ++refIt)
        {
            if ((haiku_dev_t)refIt->first.GetDevice() == it->second->GetId())
            {
                toRemove.push_back(refIt);
            }
        }
        for (auto& refIt : toRemove)
        {
            _entryRefs.erase(refIt);
        }
        _deviceMounts.erase(it);

        System::GetInstance().GetNodeMonitorService()
            .NotifyUnmount(dev);

        return B_OK;
    }
    return B_DEVICE_NOT_FOUND;
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
    if (path == path.lexically_normal())
    {
        return _DoWork(path, true, [&](std::filesystem::path& currentPath,
            const std::shared_ptr<VfsDevice>& device, bool& isSymlink)
        {
            return device->RealPath(currentPath, isSymlink);
        });
    }
    else
    {
        std::vector<std::filesystem::path> components(path.begin(), path.end());

        std::filesystem::path currentPath;

        status_t status;

        for (size_t i = 0; i < components.size(); ++i)
        {
            const auto& component = components[i];
            if (component == "..")
            {
                status = RealPath(currentPath);
                if (status != B_OK)
                {
                    path = currentPath.parent_path();
                    for (size_t j = i + 1; j < components.size(); ++j)
                    {
                        path /= components[j];
                    }
                    return status;
                }
                currentPath = currentPath.parent_path();
            }
            else if (!component.empty() && component != ".")
            {
                currentPath /= component;
            }
        }

        assert(currentPath == currentPath.lexically_normal());
        status = RealPath(currentPath);
        path = currentPath;

        return status;
    }
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

status_t VfsService::AddMonitor(haiku_dev_t device, haiku_ino_t node)
{
    auto vfsDevice = _devices.Get(device);
    if (!vfsDevice)
    {
        return B_ENTRY_NOT_FOUND;
    }
    if (_monitors[device].contains(node))
    {
        return B_OK;
    }
    status_t status = vfsDevice->AddMonitor(node);
    if (status != B_OK)
    {
        return status;
    }
    _monitors[device].insert(node);
    return B_OK;
}

status_t VfsService::RemoveMonitor(haiku_dev_t device, haiku_ino_t node)
{
    auto vfsDevice = _devices.Get(device);
    if (!vfsDevice)
    {
        return B_ENTRY_NOT_FOUND;
    }
    if (!_monitors[device].contains(node))
    {
        return B_OK;
    }
    status_t status = vfsDevice->RemoveMonitor(node);
    if (status != B_OK)
    {
        return status;
    }
    _monitors[device].erase(node);
    return B_OK;
}