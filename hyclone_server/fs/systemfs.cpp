#include <cassert>

#include "fs/systemfs.h"
#include "server_filesystem.h"

SystemfsDevice::SystemfsDevice(const std::filesystem::path& root)
    : HostfsDevice(root, "/")
{
    assert(server_setup_systemfs(_info));
}