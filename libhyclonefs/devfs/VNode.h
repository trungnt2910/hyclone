#ifndef __LIBHYCLONEFS_DEVFS_VNODE_H__
#define __LIBHYCLONEFS_DEVFS_VNODE_H__

#include "fssh_defs.h"

#include "StatData.h"

class VNode
{
    public:
        fssh_ino_t GetID() const;
        const StatData *GetStatData() const;
};

#endif // __LIBHYCLONEFS_DEVFS_VNODE_H__