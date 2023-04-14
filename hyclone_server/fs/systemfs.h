#ifndef __HYCLONE_SYSTEMFS_H__
#define __HYCLONE_SYSTEMFS_H__

#include "fs/hostfs.h"

class SystemfsDevice : public HostfsDevice
{
public:
    SystemfsDevice(const std::filesystem::path& root = "/SystemRoot",
        const std::filesystem::path& hostRoot = "/", uint32 mountFlags = 0);

    static status_t Mount(const std::filesystem::path& path,
        const std::filesystem::path& device, uint32 flags,
        const std::string& args, std::shared_ptr<VfsDevice>& output);
};

#endif // __HYCLONE_ROOTFS_H__