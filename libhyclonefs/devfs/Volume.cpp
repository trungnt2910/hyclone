#include "RootVNode.h"
#include "Volume.h"

#include "devfs.h"

#include "fssh_errors.h"
#include "fssh_stat.h"
#include "fssh_string.h"
#include "vfs.h"

#include <iostream>
#include <new>

Volume::Volume()
{
    fRootVNode = NULL;
    fFSVolume = NULL;
}

Volume::~Volume()
{
    Unmount();
}

fssh_status_t Volume::Mount(fssh_fs_volume *fsVolume, const char *path)
{
    Unmount();
    fssh_status_t error = FSSH_B_OK;

    fFSVolume = fsVolume;

    // load the settings
    // copy the device name
    // open disk
    // read and analyze superblock
    // create and init block cache
    // create the tree
    // get the root node and init the tree
    
    if (error == FSSH_B_OK)
    {
        fRootVNode = new (std::nothrow) RootVNode(this);
        if (fRootVNode == NULL)
        {
            error = FSSH_B_NO_MEMORY;
        }
        else
        {
            error = fssh_publish_vnode(fFSVolume, fRootVNode->GetID(), fRootVNode, &gDevFSVNodeOps, FSSH_S_IFDIR, 0);
        }
    }

    // init the hash function
    // init the negative entry list
    // cleanup on error
    if (error != FSSH_B_OK)
    {
        Unmount();
    }

    return error;
}

fssh_status_t Volume::Unmount()
{
    delete fRootVNode;
    fRootVNode = NULL;
    fFSVolume = NULL;
    if (fMountPoint != NULL)
    {
        FSShell::vfs_put_vnode(fMountPoint);
    }
    return FSSH_B_OK;
}

fssh_fs_vnode* Volume::GetMountPoint()
{
    if (fMountPoint == NULL)
    {
        fssh_status_t status = FSShell::vfs_entry_ref_to_vnode(fFSVolume->id, fRootVNode->GetID(), "..", (void**)&fMountPoint);
        if (status != FSSH_B_OK)
        {
            fMountPoint = NULL;
        }
    }

    return fMountPoint;
}

fssh_status_t Volume::FindVNode(fssh_ino_t id, VNode** node)
{
    std::cerr << "Volume::FindVNode() not implemented." << std::endl;
    return FSSH_B_UNSUPPORTED;
}