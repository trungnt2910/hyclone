#include "Volume.h"

#include "fssh_errors.h"

#include <iostream>

Volume::Volume()
{
    std::cerr << "Volume::Volume() not implemented." << std::endl;
}

Volume::~Volume()
{
    std::cerr << "Volume::~Volume() not implemented." << std::endl;
}

fssh_status_t Volume::Mount(fssh_fs_volume *fsVolume, const char *path)
{
    std::cerr << "Volume::Mount() not implemented." << std::endl;
    return FSSH_B_OK;
}

fssh_status_t Volume::Unmount()
{
    std::cerr << "Volume::Unmount() not implemented." << std::endl;
    return FSSH_B_OK;
}

VNode* Volume::GetRootVNode() const
{
    std::cerr << "Volume::GetRootVNode() not implemented." << std::endl;
    return NULL;
}

fssh_status_t Volume::FindVNode(fssh_ino_t id, VNode *node)
{
    std::cerr << "Volume::FindVNode() not implemented." << std::endl;
    return FSSH_B_UNSUPPORTED;
}