#ifndef __LIBHYCLONEFS_DEVFS_DEVFS_H__
#define __LIBHYCLONEFS_DEVFS_DEVFS_H__

#include "fssh_defs.h"
#include "fssh_fs_interface.h"

static fssh_status_t devfs_std_ops(int32_t op, ...);
static fssh_status_t devfs_mount(fssh_fs_volume* _volume, const char* device, uint32_t flags, const char* parameters, fssh_ino_t* rootID);
static fssh_status_t devfs_unmount(fssh_fs_volume* _volume);
static fssh_status_t devfs_read_fs_info(fssh_fs_volume* _volume, fssh_fs_info* info);
static fssh_status_t devfs_read_vnode(fssh_fs_volume* _volume, fssh_vnode_id id, fssh_fs_vnode* _node, int* type, uint32_t* flags, bool reenter);

fssh_module_info* devfs_get_module();

#endif // __LIBHYCLONEFS_DEVFS_DEVFS_H__