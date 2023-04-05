#include <cassert>

#include "fs/packagefs.h"
#include "server_filesystem.h"

PackagefsDevice::PackagefsDevice(const std::filesystem::path& root,
    const std::filesystem::path& hostRoot)
    : HostfsDevice(root, hostRoot)
{
    assert(hostRoot.filename() == "boot");
    assert(server_setup_packagefs(hostRoot.parent_path(), _info));
}