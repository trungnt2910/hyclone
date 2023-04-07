#ifndef __SERVER_VFS_H__
#define __SERVER_VFS_H__

#include <cassert>
#include <functional>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "BeDefs.h"
#include "entry_ref.h"
#include "haiku_dirent.h"
#include "haiku_errors.h"
#include "haiku_fs_info.h"
#include "haiku_stat.h"
#include "id_map.h"

struct VfsDir;

class VfsDevice
{
protected:
    haiku_fs_info _info;
    std::filesystem::path _root;
    VfsDevice(const std::filesystem::path& root) : _root(root) {}
public:
    VfsDevice(const std::filesystem::path& root, const haiku_fs_info& info);
    virtual ~VfsDevice() = default;

    // If the initial value if isSymlink is true, the function will traverse the symlinks.
    virtual status_t GetPath(std::filesystem::path& path, bool& isSymlink) = 0;
    virtual status_t ReadStat(std::filesystem::path& path, haiku_stat& stat, bool& isSymlink) = 0;
    virtual status_t WriteStat(std::filesystem::path& path, const haiku_stat& stat,
        int statMask, bool& isSymlink) = 0;
    virtual status_t OpenDir(std::filesystem::path& path, VfsDir& dir, bool& isSymlink) = 0;
    // Length of dirent passed in dirent->reclen.
    virtual status_t ReadDir(VfsDir& dir, haiku_dirent& dirent) = 0;
    virtual status_t RewindDir(VfsDir& dir) = 0;
    virtual status_t Ioctl(const std::filesystem::path& path, unsigned int cmd,
        void* addr, void* buffer, size_t size) { return B_BAD_VALUE; }

    const haiku_fs_info& GetInfo() const { return _info; }
    haiku_fs_info& GetInfo() { return _info; }
    const std::filesystem::path& GetRoot() const { return _root; }
    int GetId() const { return _info.dev; }
};

// Equivalent to the POSIX DIR type.
struct VfsDir
{
    std::filesystem::directory_iterator dir;
    std::filesystem::path hostPath;
    std::filesystem::path path;
    std::weak_ptr<VfsDevice> device;
    size_t cookie;
    haiku_dev_t dev;
    haiku_ino_t ino;
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

    typedef std::function<status_t(std::filesystem::path&, const std::shared_ptr<VfsDevice>&, bool&)> callback_t;
    status_t _DoWork(std::filesystem::path&, bool traverseLink, const callback_t& work);
    status_t _DoWork(const std::filesystem::path&, bool traverseLink, const callback_t& work);
public:
    VfsService();
    ~VfsService() = default;

    size_t RegisterEntryRef(const EntryRef& ref, const std::string& path);
    size_t RegisterEntryRef(const EntryRef& ref, std::string&& path);
    bool GetEntryRef(const EntryRef& ref, std::string& path) const;

    size_t RegisterDevice(const std::shared_ptr<VfsDevice>& device);
    std::weak_ptr<VfsDevice> GetDevice(int id);
    std::weak_ptr<VfsDevice> GetDevice(const std::filesystem::path& path);
    haiku_dev_t NextDeviceId(haiku_dev_t id) const { return _devices.NextId(id); }

    // Gets the host path for the given VFS path.
    // The VFS path MUST be absolute.
    status_t GetPath(std::filesystem::path& path, bool traverseLink = true);
    status_t RealPath(std::filesystem::path& path);
    status_t ReadStat(const std::filesystem::path& path, haiku_stat& stat, bool traverseLink = true);
    status_t WriteStat(const std::filesystem::path& path, const haiku_stat& stat,
        int statMask, bool traverseLink = true);
    status_t OpenDir(const std::filesystem::path& path, VfsDir& dir, bool traverseLink = true);
    status_t ReadDir(VfsDir& dir, haiku_dirent& dirent);
    status_t RewindDir(VfsDir& dir);
    status_t Ioctl(const std::filesystem::path& path, unsigned int cmd, void* addr, void* buffer, size_t size);

    std::unique_lock<std::recursive_mutex> Lock() { return std::unique_lock<std::recursive_mutex>(_lock); }
};

#endif // __SERVER_VFS_H__