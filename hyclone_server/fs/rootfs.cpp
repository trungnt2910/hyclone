#include <cassert>

#include "rootfs.h"
#include "server_filesystem.h"

RootfsDevice::RootfsDevice(const std::filesystem::path& hostRoot)
    : HostfsDevice("/", hostRoot)
{
    assert(server_setup_rootfs(hostRoot, _info));
}

bool RootfsDevice::_IsBlacklisted(const std::filesystem::path& hostPath) const
{
    if (hostPath.parent_path() == _hostRoot && hostPath.filename().string().starts_with(".hyclone"))
    {
        return true;
    }
    return false;
}

bool RootfsDevice::_IsBlacklisted(const std::filesystem::directory_entry& entry) const
{
    return _IsBlacklisted(entry.path());
}