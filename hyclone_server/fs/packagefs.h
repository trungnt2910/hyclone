#ifndef __HYCLONE_PACKAGEFS_H__
#define __HYCLONE_PACKAGEFS_H__

#include "fs/hostfs.h"
#include "haiku_drivers.h"

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
private:
    std::filesystem::path _relativeInstalledPackagesPath = std::filesystem::path("system/.hpkgvfsPackages");
    PackageFSMountType _mountType = PACKAGE_FS_MOUNT_TYPE_SYSTEM;
protected:
    bool _IsBlacklisted(const std::filesystem::path& path) const override;
    bool _IsBlacklisted(const std::filesystem::directory_entry& entry) const override;
public:
    PackagefsDevice(const std::filesystem::path& root,
        const std::filesystem::path& hostRoot);
    virtual status_t Ioctl(const std::filesystem::path& path, unsigned int cmd,
        void* addr, void* buffer, size_t size) override;
};

#endif // __HYCLONE_ROOTFS_H__