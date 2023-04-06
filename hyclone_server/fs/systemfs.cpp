#include <cassert>
#include <cstring>

#include "fs/systemfs.h"
#include "server_filesystem.h"
#include "server_native.h"

SystemfsDevice::SystemfsDevice(const std::filesystem::path& root)
    : HostfsDevice(root, "/")
{
    _info.flags = B_FS_IS_PERSISTENT | B_FS_IS_SHARED | B_FS_HAS_ATTR;
    _info.block_size = 0;
    _info.io_size = 0;
    _info.total_blocks = 0;
    _info.free_blocks = 0;
    _info.total_nodes = 0;
    _info.free_nodes = 0;
    strncpy(_info.volume_name, root.filename().c_str(), sizeof(_info.volume_name));
    strncpy(_info.fsh_name, "systemfs", sizeof(_info.fsh_name));

    server_fill_fs_info("/", &_info);
}