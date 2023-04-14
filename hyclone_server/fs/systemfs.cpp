#include <cassert>
#include <cstring>

#include "fs/systemfs.h"
#include "server_filesystem.h"
#include "server_native.h"

SystemfsDevice::SystemfsDevice(const std::filesystem::path& root,
    const std::filesystem::path& hostRoot, uint32 mountFlags)
    : HostfsDevice(root, hostRoot, mountFlags)
{
    _info.flags = B_FS_IS_PERSISTENT | B_FS_IS_SHARED;
    _info.block_size = 0;
    _info.io_size = 0;
    _info.total_blocks = 0;
    _info.free_blocks = 0;
    _info.total_nodes = 0;
    _info.free_nodes = 0;
    strncpy(_info.volume_name, root.filename().c_str(), sizeof(_info.volume_name));
    strncpy(_info.fsh_name, "systemfs", sizeof(_info.fsh_name));

    server_fill_fs_info(hostRoot, &_info);
}

status_t SystemfsDevice::Mount(const std::filesystem::path& path,
    const std::filesystem::path& device, uint32 flags, const std::string& args,
    std::shared_ptr<VfsDevice>& output)
{
    std::filesystem::path hostRoot = "/";

    if (!args.empty())
    {
        hostRoot = args;

        if (!hostRoot.is_absolute())
        {
            return B_BAD_VALUE;
        }

        if (!std::filesystem::exists(hostRoot))
        {
            return B_ENTRY_NOT_FOUND;
        }
    }

    output = std::make_shared<SystemfsDevice>(path, hostRoot, flags);

    return B_OK;
}