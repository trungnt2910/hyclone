#ifndef __HYCLONE_ROOTFS_H__
#define __HYCLONE_ROOTFS_H__

#include "fs/hostfs.h"

class RootfsDevice : public HostfsDevice
{
protected:
    virtual bool _IsBlacklisted(const std::filesystem::path& path) const override;
    virtual bool _IsBlacklisted(const std::filesystem::directory_entry& entry) const override;
public:
    RootfsDevice(const std::filesystem::path& hostRoot);
};

#endif // __HYCLONE_ROOTFS_H__