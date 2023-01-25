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
    NULL
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
    VNode* foundNode = new (std::nothrow) VNode;

    if (foundNode)
    {
        error = volume->FindVNode(id, foundNode);
        if (error == FSSH_B_OK)
        {
            _node->private_node = foundNode;
            _node->ops = &gDevFSVNodeOps;
            *type = foundNode->GetStatData()->GetID() & FSSH_S_IFMT;
            *flags = 0;
        }
        else
        {
            delete foundNode;
        }
    }
    else
    {
        error = FSSH_B_NO_MEMORY;
    }

    return FSSH_B_OK;
}