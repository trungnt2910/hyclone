#include <cassert>

#include "fs/devfs.h"
#include "server_filesystem.h"

DevfsDevice::DevfsDevice()
    : HostfsDevice("/dev", "/dev")
{
    assert(server_setup_devfs(_info));
}