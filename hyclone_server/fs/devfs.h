#ifndef __HYCLONE_DEVFS_H__
#define __HYCLONE_DEVFS_H__

#include "fs/hostfs.h"

class DevfsDevice : public HostfsDevice
{
public:
    DevfsDevice(uint32 mountFlags = 0);
};

#endif // __HYCLONE_ROOTFS_H__