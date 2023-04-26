#ifndef __SERVER_VFS_H__
#define __SERVER_VFS_H__

#include <cassert>
#include <functional>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "BeDefs.h"
#include "entry_ref.h"
#include "haiku_dirent.h"
#include "haiku_errors.h"
#include "haiku_fs_attr.h"
#include "haiku_fs_info.h"
#include "haiku_stat.h"
#include "id_map.h"

class VfsDevice
{
protected:
    haiku_fs_info _info;
    std::filesystem::path _root;
    uint32 _mountFlags;
    VfsDevice(const std::filesystem::path& root, uint32 mountFlags)
        : _root(root), _mountFlags(mountFlags) { }
public:
    VfsDevice(const std::filesystem::path& root, const haiku_fs_info& info, uint32 mountFlags)
        : _root(root), _info(info), _mountFlags(mountFlags) { }
    virtual ~VfsDevice() = default;

    // If the initial value if isSymlink is true, the function will traverse the symlinks.
    virtual status_t GetPath(std::filesystem::path& path, bool& isSymlink) = 0;
    virtual status_t GetAttrPath(std::filesystem::path& path, const std::string& name,
        uint32 type, bool createNew, bool& isSymlink) { return B_UNSUPPORTED; }
    virtual status_t RealPath(std::filesystem::path& path, bool& isSymlink) = 0;
    virtual status_t ReadStat(std::filesystem::path& path, haiku_stat& stat, bool& isSymlink) = 0;
    virtual status_t WriteStat(std::filesystem::path& path, const haiku_stat& stat,
        int statMask, bool& isSymlink) = 0;
    virtual status_t StatAttr(const std::filesystem::path& path, const std::string& name,
        haiku_attr_info& info) { return B_UNSUPPORTED; }
    virtual status_t TransformDirent(const std::filesystem::path& path, haiku_dirent& dirent) = 0;
    virtual haiku_ssize_t ReadAttr(const std::filesystem::path& path, const std::string& name, size_t pos,
        void* buffer, size_t size) { return B_UNSUPPORTED; }
    virtual haiku_ssize_t WriteAttr(const std::filesystem::path& path, const std::string& name, uint32 type,
        size_t pos, const void* buffer, size_t size) { return B_UNSUPPORTED; }
    virtual haiku_ssize_t RemoveAttr(const std::filesystem::path& path, const std::string& name)
        { return B_UNSUPPORTED; }
    virtual status_t Ioctl(const std::filesystem::path& path, unsigned int cmd,
        void* addr, void* buffer, size_t size) { return B_BAD_VALUE; }

    virtual status_t AddMonitor(haiku_ino_t node) { return B_UNSUPPORTED; }
    virtual status_t RemoveMonitor(haiku_ino_t node) { return B_UNSUPPORTED; }

    const haiku_fs_info& GetInfo() const { return _info; }
    haiku_fs_info& GetInfo() { return _info; }
    const std::filesystem::path& GetRoot() const { return _root; }
    haiku_dev_t GetId() const { return _info.dev; }
};

class VfsService
{
private:
    struct PathHash
    {
        size_t operator()(const std::filesystem::path& path) const
        {
            assert(path.is_absolute());
            assert(path.lexically_normal() == path);
            return std::filesystem::hash_value(path);
        }
    };

    std::recursive_mutex _lock;
    std::unordered_map<EntryRef, std::string> _entryRefs;
    IdMap<std::shared_ptr<VfsDevice>, haiku_dev_t> _devices;
    std::unordered_map<std::filesystem::path, std::shared_ptr<VfsDevice>, PathHash> _deviceMounts;
    std::unordered_map<int, int> _deviceReferences;
    std::unordered_map<haiku_dev_t, std::unordered_set<haiku_ino_t>> _monitors;
    using mounter_t = status_t(*)(const std::filesystem::path& path, const std::filesystem::path& device,
        uint32 mountFlags, const std::string& args, std::shared_ptr<VfsDevice>& output);
    std::unordered_map<std::string, mounter_t> _mounters;

    template <typename T = status_t>
    struct Callback
    {
        typedef std::function<T(std::filesystem::path&, const std::shared_ptr<VfsDevice>&, bool&)> Type;
    };

    template <typename T = status_t>
    T _DoWork(std::filesystem::path& path, bool traverseLink, const Callback<T>::Type& work)
    {
        path = path.lexically_normal();
        if (path.filename().empty())
        {
            path = path.parent_path();
        }

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
                isSymlink = traverseLink;
                T status = work(path, device, isSymlink);
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
    template <typename T = status_t>
    T _DoWork(const std::filesystem::path& path, bool traverseLink, const Callback<T>::Type& work)
    {
        auto tmp = path;
        return _DoWork<T>(tmp, traverseLink, work);
    }
    template <typename T = status_t>
    T _DoWork(std::filesystem::path& path, const Callback<T>::Type& work)
    {
        return _DoWork<T>(path, false, work);
    }
    template <typename T = status_t>
    T _DoWork(const std::filesystem::path& path, const Callback<T>::Type& work)
    {
        return _DoWork<T>(path, false, work);
    }
public:
    VfsService();
    ~VfsService() = default;

    size_t RegisterEntryRef(const EntryRef& ref, const std::string& path);
    size_t RegisterEntryRef(const EntryRef& ref, std::string&& path);
    size_t UnregisterEntryRef(const EntryRef& ref);
    bool GetEntryRef(const EntryRef& ref, std::string& path) const;
    bool SearchEntryRef(const std::string& path, EntryRef& ref) const;

    size_t RegisterDevice(const std::shared_ptr<VfsDevice>& device);
    std::weak_ptr<VfsDevice> GetDevice(int id);
    std::weak_ptr<VfsDevice> GetDevice(const std::filesystem::path& path);
    haiku_dev_t NextDeviceId(haiku_dev_t id) const { return _devices.NextId(id); }

    void RegisterBuiltinFilesystem(const std::string& name, mounter_t mounter);
    haiku_dev_t Mount(const std::filesystem::path& path, const std::filesystem::path& device,
        const std::string& fsName, uint32 flags, const std::string& args);
    status_t Unmount(const std::filesystem::path& path, uint32 flags);

    // Gets the host path for the given VFS path.
    // The VFS path MUST be absolute.
    status_t GetPath(std::filesystem::path& path, bool traverseLink = true);
    status_t GetAttrPath(std::filesystem::path& path, const std::string& name, uint32 type,
        bool createNew, bool traverseLink = true);
    status_t RealPath(std::filesystem::path& path);
    status_t ReadStat(const std::filesystem::path& path, haiku_stat& stat, bool traverseLink = true);
    status_t WriteStat(const std::filesystem::path& path, const haiku_stat& stat,
        int statMask, bool traverseLink = true);
    status_t StatAttr(const std::filesystem::path& path, const std::string& name, haiku_attr_info& info);
    status_t TransformDirent(const std::filesystem::path& path, haiku_dirent& dirent);
    haiku_ssize_t ReadAttr(const std::filesystem::path& path, const std::string& name, size_t pos,
        void* buffer, size_t size);
    haiku_ssize_t WriteAttr(const std::filesystem::path& path, const std::string& name, uint32 type,
        size_t pos, const void* buffer, size_t size);
    status_t RemoveAttr(const std::filesystem::path& path, const std::string& name);
    status_t Ioctl(const std::filesystem::path& path, unsigned int cmd, void* addr, void* buffer, size_t size);

    status_t AddMonitor(haiku_dev_t device, haiku_ino_t node);
    status_t RemoveMonitor(haiku_dev_t device, haiku_ino_t node);

    std::unique_lock<std::recursive_mutex> Lock() { return std::unique_lock<std::recursive_mutex>(_lock); }
};

#endif // __SERVER_VFS_H__