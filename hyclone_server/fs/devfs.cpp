#include <cassert>
#include <cstring>

#include "fs/devfs.h"
#include "server_filesystem.h"
#include "server_native.h"

DevfsDevice::DevfsDevice()
    : HostfsDevice("/dev", "/dev")
{
    // same as in Haiku.
    _info.flags = 0;
    _info.block_size = 0;
    _info.io_size = 0;
    _info.total_blocks = 0;
    _info.free_blocks = 0;
    _info.total_nodes = 0;
    _info.free_nodes = 0;
    _info.device_name[0] = '\0';
    _info.volume_name[0] = '\0';
    strncpy(_info.fsh_name, "devfs", sizeof(_info.fsh_name));
}