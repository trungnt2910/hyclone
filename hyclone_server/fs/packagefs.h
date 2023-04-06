#ifndef __HYCLONE_PACKAGEFS_H__
#define __HYCLONE_PACKAGEFS_H__

#include "fs/hostfs.h"

class PackagefsDevice : public HostfsDevice
{
private:
    std::filesystem::path _relativeInstalledPackagesPath = std::filesystem::path("system/.hpkgvfsPackages");
protected:
    bool _IsBlacklisted(const std::filesystem::path& path) const override;
    bool _IsBlacklisted(const std::filesystem::directory_entry& entry) const override;
public:
    PackagefsDevice(const std::filesystem::path& root,
        const std::filesystem::path& hostRoot);
};

#endif // __HYCLONE_ROOTFS_H__