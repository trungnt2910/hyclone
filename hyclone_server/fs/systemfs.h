#ifndef __HYCLONE_SYSTEMFS_H__
#define __HYCLONE_SYSTEMFS_H__

#include "fs/hostfs.h"

class SystemfsDevice : public HostfsDevice
{
public:
    SystemfsDevice(const std::filesystem::path& root = "/SystemRoot");
};

#endif // __HYCLONE_ROOTFS_H__