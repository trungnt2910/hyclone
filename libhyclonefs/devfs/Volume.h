#ifndef __LIBHYCLONEFS_DEVFS_VOLUME_H__
#define __LIBHYCLONEFS_DEVFS_VOLUME_H__

#include "fssh_defs.h"
#include "fssh_fs_interface.h"

class VNode;

class Volume
{
    public:
        Volume();
        ~Volume();

        fssh_status_t Mount(fssh_fs_volume *fsVolume, const char *path);
        fssh_status_t Unmount();

        VNode* GetRootVNode() const;
        fssh_status_t FindVNode(fssh_ino_t id, VNode *node);
};

#endif // __LIBHYCLONEFS_DEVFS_VOLUME_H__