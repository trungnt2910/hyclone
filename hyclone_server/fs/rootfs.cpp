#include <cassert>
#include <cstring>

#include "rootfs.h"
#include "server_filesystem.h"

RootfsDevice::RootfsDevice(const std::filesystem::path& hostRoot, uint32 mountFlags)
    : HostfsDevice("/", hostRoot, mountFlags)
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
    strncpy(_info.fsh_name, "rootfs", sizeof(_info.fsh_name));

    auto binPath = hostRoot / "bin";
    auto libPath = hostRoot / "lib";
    auto etcPath = hostRoot / "etc";
    auto packagesPath = hostRoot / "packages";

    // Generate some essential symlinks.
    std::filesystem::remove(binPath);
    std::filesystem::create_directory_symlink("boot/system/bin", binPath);
    std::filesystem::remove(libPath);
    std::filesystem::create_directory_symlink("boot/system/lib", libPath);
    std::filesystem::remove(etcPath);
    std::filesystem::create_directory_symlink("boot/system/etc", etcPath);
    std::filesystem::remove(packagesPath);
    std::filesystem::create_directory_symlink("boot/system/package-links", packagesPath);
}

bool RootfsDevice::_IsBlacklisted(const std::filesystem::path& hostPath) const
{
    std::string path = hostPath.lexically_normal().string();
    std::string blacklistedPrefix = (_hostRoot / ".hyclone").string();

    return path.compare(0, blacklistedPrefix.size(), blacklistedPrefix) == 0;
}

bool RootfsDevice::_IsBlacklisted(const std::filesystem::directory_entry& entry) const
{
    return _IsBlacklisted(entry.path());
}