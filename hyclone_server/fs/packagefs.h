#ifndef __HYCLONE_PACKAGEFS_H__
#define __HYCLONE_PACKAGEFS_H__

#include "fs/hostfs.h"

class PackagefsDevice : public HostfsDevice
{
public:
    PackagefsDevice(const std::filesystem::path& root,
        const std::filesystem::path& hostRoot);
};

#endif // __HYCLONE_ROOTFS_H__