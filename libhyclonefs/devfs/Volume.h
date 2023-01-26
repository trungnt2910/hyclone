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

        fssh_status_t Mount(fssh_fs_volume* fsVolume, const char* path);
        fssh_status_t Unmount();

        VNode* GetRootVNode() const { return fRootVNode; }
        fssh_status_t FindVNode(fssh_ino_t id, VNode** node);
        fssh_fs_vnode* GetMountPoint();
        fssh_dev_t GetID() const { return fFSVolume->id; }
        fssh_fs_volume* GetFSVolume() const { return fFSVolume; }

    private:
        fssh_fs_volume* fFSVolume;
        VNode* fRootVNode;
        fssh_fs_vnode* fMountPoint;
};

#endif // __LIBHYCLONEFS_DEVFS_VOLUME_H__