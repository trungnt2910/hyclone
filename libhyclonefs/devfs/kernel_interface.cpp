#include "devfs.h"
#include "Volume.h"
#include "VNode.h"

#include <iostream>
#include <new>

#include "fssh_errors.h"
#include "fssh_fs_info.h"
#include "fssh_fs_volume.h"
#include "fssh_stat.h"
#include "fssh_string.h"
#include "fssh_unistd.h"

static fssh_file_system_module_info sDevFSModuleInfo = {
    {
        "file_systems/devfs" FSSH_B_CURRENT_FS_API_VERSION,
        0,
        devfs_std_ops,
    },

    "devfs",                      // short_name
    "Hyclone device file system", // pretty_name
    0,                            // DDM flags

    // scanning
    NULL, // identify_partition()
    NULL, // scan_partition()
    NULL, // free_identify_partition_cookie(),
    NULL, // free_partition_content_cookie()

    devfs_mount
};

fssh_fs_volume_ops gDevFSVolumeOps = {
    &devfs_unmount,
    &devfs_read_fs_info,
    NULL,    // write_fs_info,
    NULL,    // sync,

    &devfs_read_vnode
};

fssh_fs_vnode_ops gDevFSVNodeOps = {
    /* vnode operations */
    &devfs_lookup,
    NULL,	// &get_vnode_name,
    &devfs_write_vnode,
    NULL,	// &remove_vnode,

    /* VM file access */
    NULL,	// &can_page,
    NULL,	// &read_pages,
    NULL,	// &write_pages,

    NULL,	// io()
    NULL,	// cancel_io()

    NULL,	// &get_file_map,

    NULL,	// &ioctl,
    NULL,	// &set_flags,
    NULL,	// &select,
    NULL,	// &deselect,
    NULL,	// &fsync,

    &devfs_read_symlink,
    NULL,	// &create_symlink,

    NULL,	// &link,
    NULL,	// &unlink,
    NULL,	// &rename,

    &devfs_access,
    &devfs_read_stat,
    NULL,	// &write_stat,
    NULL,	// &preallocate,

    /* file operations */
    NULL,	// &create,
    &devfs_open,
    &devfs_close,
    &devfs_free_cookie,
    &devfs_read,
    &devfs_write,

    /* directory operations */
    NULL,	// &create_dir,
    NULL,	// &remove_dir,
    &devfs_open_dir,
    &devfs_close_dir,
    &devfs_free_dir_cookie,
    &devfs_read_dir,
    &devfs_rewind_dir
};

#ifdef LIBHYCLONEFS_MODULE
fssh_module_info* modules[] = {
    (fssh_module_info*)&sDevFSModuleInfo,
    NULL
};
#else
fssh_module_info* devfs_get_module()
{
    return (fssh_module_info*)&sDevFSModuleInfo;
}
#endif

static fssh_status_t devfs_std_ops(int32_t op, ...)
{
    switch (op)
    {
        case FSSH_B_MODULE_INIT:
        {
            return FSSH_B_OK;
        }
        case FSSH_B_MODULE_UNINIT:
        {
            return FSSH_B_OK;
        }
        default:
        {
            return FSSH_B_ERROR;
        }
    }
}

static fssh_status_t devfs_mount(fssh_fs_volume* _volume, const char* device, uint32_t flags, const char* parameters, fssh_ino_t* rootID)
{
    (void)parameters;
    (void)flags;

    fssh_status_t error = FSSH_B_OK;

    Volume* volume = new (std::nothrow) Volume;
    if (volume == NULL)
    {
        error = FSSH_B_NO_MEMORY;
    }

    if (error == FSSH_B_OK)
    {
        error = volume->Mount(_volume, device);
    }

    if (error == FSSH_B_OK)
    {
        *rootID = volume->GetRootVNode()->GetID();
        _volume->private_volume = volume;
        _volume->ops = &gDevFSVolumeOps;
    }

    if (error != FSSH_B_OK)
    {
        delete volume;
    }

    return FSSH_B_OK;
}

static fssh_status_t devfs_unmount(fssh_fs_volume* _volume)
{
    Volume* volume = (Volume*)_volume->private_volume;
    fssh_status_t error = volume->Unmount();
 
    if (error == FSSH_B_OK)
    {
        delete volume;
    }
    return error;
}

static fssh_status_t devfs_read_fs_info(fssh_fs_volume* _volume, fssh_fs_info* info)
{
    info->flags = 0;
    info->io_size = FSSH_B_PAGE_SIZE;
    info->block_size = FSSH_B_PAGE_SIZE;
    info->total_blocks = 1;
    info->free_blocks = 0;
    fssh_strlcpy(info->device_name, "Hyclone", sizeof(info->device_name));
    fssh_strlcpy(info->volume_name, "Emulated Devices", sizeof(info->volume_name));
    return FSSH_B_OK;
}

static fssh_status_t devfs_read_vnode(fssh_fs_volume* _volume, fssh_vnode_id id, fssh_fs_vnode* _node, int* type, uint32_t* flags, bool reenter)
{
    (void)reenter;

    Volume* volume = (Volume*)_volume->private_volume;

    fssh_status_t error = FSSH_B_OK;
    VNode* foundNode;

    error = volume->FindVNode(id, &foundNode);
    if (error == FSSH_B_OK)
    {
        _node->private_node = foundNode;
        _node->ops = &gDevFSVNodeOps;
        *type = foundNode->GetStatData()->fssh_st_mode & FSSH_S_IFMT;
        *flags = 0;
    }

    return error;
}

static fssh_status_t devfs_lookup(fssh_fs_volume* _volume, fssh_fs_vnode* _dir, const char* name, fssh_vnode_id* vnid)
{
    (void)_volume;
    VNode* dir = (VNode*)_dir->private_node;

    fssh_status_t error = FSSH_B_OK;

    error = dir->Lookup(name, vnid);

    return error;
}

static fssh_status_t devfs_write_vnode(fssh_fs_volume *fs, fssh_fs_vnode *_node, bool reenter)
{
    std::cerr << "devfs_write_vnode not implemented" << std::endl;
    return FSSH_B_NOT_SUPPORTED;
}

static fssh_status_t devfs_read_symlink(fssh_fs_volume* _volume, fssh_fs_vnode* _node, char* buffer, size_t* bufferSize)
{
    std::cerr << "devfs_read_symlink not implemented" << std::endl;
    return FSSH_B_NOT_SUPPORTED;
}

static fssh_status_t devfs_access(fssh_fs_volume* _volume, fssh_fs_vnode* _node, int mode)
{
    (void)_volume;
    VNode* node = (VNode*)_node->private_node;

    int perms = node->GetStatData()->fssh_st_mode & FSSH_S_IUMSK;
    if ((mode & FSSH_W_OK) && (perms & FSSH_S_IWUSR) == 0)
    {
        return FSSH_B_NOT_ALLOWED;
    }
    if ((mode & FSSH_X_OK) && (perms & FSSH_S_IXUSR) == 0)
    {
        return FSSH_B_NOT_ALLOWED;
    }
    if ((mode & FSSH_R_OK) && (perms & FSSH_S_IRUSR) == 0)
    {
        return FSSH_B_NOT_ALLOWED;
    }
    return FSSH_B_OK;
}

static fssh_status_t devfs_read_stat(fssh_fs_volume* _volume, fssh_fs_vnode* _node, struct fssh_stat* stat)
{
    (void)_volume;
    VNode* node = (VNode*)_node->private_node;

    fssh_status_t error = FSSH_B_OK;

    fssh_memcpy(stat, node->GetStatData(), sizeof(struct fssh_stat));

    return error;
}

static fssh_status_t devfs_open(fssh_fs_volume* _volume, fssh_fs_vnode* _node, int openMode, void** cookie)
{
    (void)_volume;
    VNode* node = (VNode*)_node->private_node;

    fssh_status_t error = FSSH_B_OK;

    error = node->Open(openMode, cookie);

    return error;
}

static fssh_status_t devfs_close(fssh_fs_volume* _volume, fssh_fs_vnode* _node, void* cookie)
{
    (void)_volume;
    VNode* node = (VNode*)_node->private_node;

    fssh_status_t error = FSSH_B_OK;

    error = node->Close();

    return error;
}

static fssh_status_t devfs_free_cookie(fssh_fs_volume* _volume, fssh_fs_vnode* _node, void* cookie)
{
    (void)_volume;
    VNode* node = (VNode*)_node->private_node;

    fssh_status_t error = FSSH_B_OK;

    error = node->FreeCookie(cookie);

    return error;
}

static fssh_status_t devfs_read(fssh_fs_volume* _volume, fssh_fs_vnode* _node, void* cookie, fssh_off_t pos, void* buffer, size_t* length)
{
    (void)_volume;
    VNode* node = (VNode*)_node->private_node;

    fssh_status_t error = FSSH_B_OK;

    error = node->Read(cookie, pos, buffer, length);

    return error;
}

static fssh_status_t devfs_write(fssh_fs_volume* _volume, fssh_fs_vnode* _node, void* cookie, fssh_off_t pos, const void* buffer, size_t* length)
{
    (void)_volume;
    VNode* node = (VNode*)_node->private_node;

    fssh_status_t error = FSSH_B_OK;

    error = node->Write(cookie, pos, buffer, length);

    return error;
}

static fssh_status_t devfs_open_dir(fssh_fs_volume* _volume, fssh_fs_vnode* _node, void** cookie)
{
    Volume* volume = (Volume*)_volume->private_volume;
    VNode* dir = (VNode*)_node->private_node;

    fssh_status_t error = FSSH_B_OK;
    
    if (!dir->IsDir())
    {
        error = FSSH_B_NOT_A_DIRECTORY;
    }

    if (error == FSSH_B_OK)
    {
        int* index = new (std::nothrow) int;
        if (index)
        {
            *cookie = index;
            *index = -2;
        }
        else
        {
            error = FSSH_B_NO_MEMORY;
        }
    }

    return error;
}

static fssh_status_t devfs_close_dir(fssh_fs_volume* _volume, fssh_fs_vnode* _node, void* cookie)
{
    (void)_volume;
    (void)_node;
    (void)cookie;
    return FSSH_B_OK;
}

static fssh_status_t devfs_free_dir_cookie(fssh_fs_volume* _volume, fssh_fs_vnode* _node, void* cookie)
{
    int* index = (int*)cookie;
    delete index;
    return FSSH_B_OK;
}

static fssh_status_t devfs_read_dir(fssh_fs_volume* _volume, fssh_fs_vnode* _node, void* cookie, struct fssh_dirent* buffer, size_t bufferSize, uint32_t* num)
{
    VNode* dir = (VNode*)_node->private_node;
    int* index = (int*)cookie;

    return dir->ReadDir(index, buffer, bufferSize, num);
}

static fssh_status_t devfs_rewind_dir(fssh_fs_volume* _volume, fssh_fs_vnode* _node, void* cookie)
{
    int* index = (int*)cookie;
    *index = -2;
    return FSSH_B_OK;
}