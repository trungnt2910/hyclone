#ifndef __LIBHYCLONEFS_DEVFS_DEVFS_H__
#define __LIBHYCLONEFS_DEVFS_DEVFS_H__

#include "fssh_defs.h"
#include "fssh_fs_interface.h"

static fssh_status_t devfs_std_ops(int32_t op, ...);
static fssh_status_t devfs_mount(fssh_fs_volume* _volume, const char* device, uint32_t flags, const char* parameters, fssh_ino_t* rootID);

static fssh_status_t devfs_unmount(fssh_fs_volume* _volume);
static fssh_status_t devfs_read_fs_info(fssh_fs_volume* _volume, fssh_fs_info* info);
static fssh_status_t devfs_read_vnode(fssh_fs_volume* _volume, fssh_vnode_id id, fssh_fs_vnode* _node, int* type, uint32_t* flags, bool reenter);

static fssh_status_t devfs_lookup(fssh_fs_volume* fs, fssh_fs_vnode* _dir, const char* entryName, fssh_ino_t* vnid);
static fssh_status_t devfs_write_vnode(fssh_fs_volume* fs, fssh_fs_vnode* _node, bool reenter);
static fssh_status_t devfs_read_symlink(fssh_fs_volume* fs, fssh_fs_vnode* _node, char* buffer, fssh_size_t* bufferSize);
static fssh_status_t devfs_access(fssh_fs_volume* fs, fssh_fs_vnode* _node, int mode);
static fssh_status_t devfs_read_stat(fssh_fs_volume* fs, fssh_fs_vnode* _node, struct fssh_stat* st);
static fssh_status_t devfs_open(fssh_fs_volume* fs, fssh_fs_vnode* _node, int openMode, void* *cookie);
static fssh_status_t devfs_close(fssh_fs_volume* fs, fssh_fs_vnode* _node, void* cookie);
static fssh_status_t devfs_free_cookie(fssh_fs_volume* fs, fssh_fs_vnode* _node, void* cookie);
static fssh_status_t devfs_read(fssh_fs_volume* fs, fssh_fs_vnode* _node, void* cookie, fssh_off_t pos, void* buffer, fssh_size_t* bufferSize);
static fssh_status_t devfs_write(fssh_fs_volume* fs, fssh_fs_vnode* _node, void* cookie, fssh_off_t pos, const void* buffer, fssh_size_t* bufferSize);
static fssh_status_t devfs_open_dir(fssh_fs_volume* fs, fssh_fs_vnode* _node, void* *cookie);
static fssh_status_t devfs_close_dir(fssh_fs_volume* fs, fssh_fs_vnode* _node, void* cookie);
static fssh_status_t devfs_free_dir_cookie(fssh_fs_volume* fs, fssh_fs_vnode* _node, void* cookie);
static fssh_status_t devfs_read_dir(fssh_fs_volume* fs, fssh_fs_vnode* _node, void* cookie, struct fssh_dirent* buffer, fssh_size_t bufferSize, uint32_t* _num);
static fssh_status_t devfs_rewind_dir(fssh_fs_volume* fs, fssh_fs_vnode* _node, void* cookie);

extern fssh_fs_vnode_ops gDevFSVNodeOps;

fssh_module_info* devfs_get_module();

#endif // __LIBHYCLONEFS_DEVFS_DEVFS_H__