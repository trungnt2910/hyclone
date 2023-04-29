#ifndef __HYCLONE_PACKAGEFS_H__
#define __HYCLONE_PACKAGEFS_H__

#include "fs/hostfs.h"
#include "haiku_drivers.h"

namespace HpkgVfs
{
    class Package;
}

enum PackageFSMountType
{
    PACKAGE_FS_MOUNT_TYPE_SYSTEM,
    PACKAGE_FS_MOUNT_TYPE_HOME,
    PACKAGE_FS_MOUNT_TYPE_CUSTOM,

    PACKAGE_FS_MOUNT_TYPE_ENUM_COUNT
};

enum
{
    PACKAGE_FS_OPERATION_GET_VOLUME_INFO = B_DEVICE_OP_CODES_END + 1,
    PACKAGE_FS_OPERATION_GET_PACKAGE_INFOS,
    PACKAGE_FS_OPERATION_CHANGE_ACTIVATION
};

// PACKAGE_FS_OPERATION_GET_VOLUME_INFO

struct PackageFSDirectoryInfo
{
    // node_ref of the directory
    haiku_dev_t deviceID;
    haiku_ino_t nodeID;
};

struct PackageFSVolumeInfo
{
    PackageFSMountType mountType;

    // device and node id of the respective package FS root scope (e.g. "/boot"
    // for the three standard volumes)
    haiku_dev_t rootDeviceID;
    haiku_ino_t rootDirectoryID;

    // packageCount is set to the actual packages directory count, even if it is
    // greater than the array, so the caller can determine whether the array was
    // large enough.
    // The directories are ordered from the most recent state (the actual
    // "packages" directory) to the oldest one, the one that is actually active.
    uint32 packagesDirectoryCount;
    PackageFSDirectoryInfo packagesDirectoryInfos[1];
};

// PACKAGE_FS_OPERATION_GET_PACKAGE_INFOS

struct PackageFSPackageInfo
{
    // node_ref and entry_ref of the package file
    haiku_dev_t packageDeviceID;
    haiku_dev_t directoryDeviceID;
    haiku_ino_t packageNodeID;
    haiku_ino_t directoryNodeID;
    const char *name;
};

struct PackageFSGetPackageInfosRequest
{
    // Filled in by the FS. bufferSize is set to the required buffer size, even
    // even if the provided buffer is smaller.
    uint32 bufferSize;
    uint32 packageCount;
    PackageFSPackageInfo infos[1];
};

// PACKAGE_FS_OPERATION_CHANGE_ACTIVATION

enum PackageFSActivationChangeType
{
    PACKAGE_FS_ACTIVATE_PACKAGE,
    PACKAGE_FS_DEACTIVATE_PACKAGE,
    PACKAGE_FS_REACTIVATE_PACKAGE
};

struct PackageFSActivationChangeItem
{
    PackageFSActivationChangeType type;

    // node_ref of the package file
    haiku_dev_t packageDeviceID;
    haiku_ino_t packageNodeID;

    // entry_ref of the package file
    uint32 nameLength;
    haiku_dev_t parentDeviceID;
    haiku_ino_t parentDirectoryID;
    char *name;
    // must point to a location within the
    // request
};

struct PackageFSActivationChangeRequest
{
    uint32 itemCount;
    PackageFSActivationChangeItem items[0];
};

class PackagefsDevice : public HostfsDevice
{
    friend class PackagefsEntryWriter;
private:
    enum
    {
        PACKAGEFS_ATTREMU_FILE = 'f',
        PACKAGEFS_ATTREMU_ATTR = 'a',
        PACKAGEFS_ATTREMU_TYPE = 't'
    };

    static std::filesystem::path _relativeInstalledPackagesPath;
    static std::filesystem::path _relativeAttributesPath;
    static std::filesystem::path _relativePackagesPath;
    static std::filesystem::path _GetAttrPathInternal(
        const std::filesystem::path& path, const std::string& name);
    static std::string _UnescapeAttrName(const std::string& name);
    static status_t _ResolvePackagePath(std::filesystem::path& path);

    std::mutex _updateMutex;
    PackageFSMountType _mountType = PACKAGE_FS_MOUNT_TYPE_SYSTEM;
    std::filesystem::perms _originalPermissions;

    void _CleanupAttributes();
    std::shared_ptr<HpkgVfs::Package> _GetPackageInPackagesDirectory(const std::string& name);
protected:
    bool _IsBlacklisted(const std::filesystem::path& path) const override;
    bool _IsBlacklisted(const std::filesystem::directory_entry& entry) const override;
public:
    PackagefsDevice(const std::filesystem::path& root,
        const std::filesystem::path& hostRoot, PackageFSMountType mountType = PACKAGE_FS_MOUNT_TYPE_SYSTEM,
        uint32 mountFlags = 0);
    virtual status_t GetAttrPath(std::filesystem::path& path, const std::string& name,
        uint32 type, bool createNew, bool& isSymlink) override;
    virtual status_t StatAttr(const std::filesystem::path& path, const std::string& name,
        haiku_attr_info& info) override;
    virtual status_t TransformDirent(const std::filesystem::path& path, haiku_dirent& dirent) override;
    virtual haiku_ssize_t ReadAttr(const std::filesystem::path& path, const std::string& name, size_t pos,
        void* buffer, size_t size) override;
    virtual haiku_ssize_t WriteAttr(const std::filesystem::path& path, const std::string& name, uint32 type,
        size_t pos, const void* buffer, size_t size) override;
    virtual haiku_ssize_t RemoveAttr(const std::filesystem::path& path, const std::string& name)
        override;
    virtual status_t Ioctl(const std::filesystem::path& path, unsigned int cmd,
        void* addr, void* buffer, size_t size) override;

    virtual status_t Cleanup() override;

    static status_t Mount(const std::filesystem::path& path, const std::filesystem::path& device, uint32 flags,
        const std::string& args, std::shared_ptr<VfsDevice>& output);
};

#endif // __HYCLONE_ROOTFS_H__