#include "haiku_errors.h"
#include "hostfs.h"

HostfsDevice::HostfsDevice(const std::filesystem::path& root, const std::filesystem::path& hostRoot)
    : VfsDevice(root), _hostRoot(hostRoot)
{

}

status_t HostfsDevice::GetPath(std::filesystem::path& path, bool& isSymlink)
{
    auto relativePath = path.lexically_relative(_root);
    if (relativePath.empty())
    {
        return B_ENTRY_NOT_FOUND;
    }

    auto hostPath = _hostRoot / relativePath;
    if (!std::filesystem::exists(hostPath))
    {
        return B_ENTRY_NOT_FOUND;
    }

    if (isSymlink)
    {
        isSymlink = std::filesystem::is_symlink(hostPath);
        if (isSymlink)
        {
            std::error_code ec;
            auto symlinkPath = std::filesystem::read_symlink(path, ec);
            if (ec)
            {
                return B_ENTRY_NOT_FOUND;
            }
            if (symlinkPath.is_absolute())
            {
                path = symlinkPath;
            }
            else
            {
                path = path.parent_path() / symlinkPath;
            }
            return B_OK;
        }
    }

    path = hostPath;
    return B_OK;
}

status_t HostfsDevice::ReadStat(std::filesystem::path& path, haiku_stat& stat, bool& isSymlink)
{
    return HAIKU_POSIX_ENOSYS;
}

status_t HostfsDevice::WriteStat(std::filesystem::path& path, const haiku_stat& stat,
    int statMask, bool& isSymlink)
{
    return HAIKU_POSIX_ENOSYS;
}

status_t HostfsDevice::OpenDir(std::filesystem::path& path, VfsDir& dir, bool& isSymlink)
{
    return HAIKU_POSIX_ENOSYS;
}

status_t HostfsDevice::ReadDir(VfsDir& dir, haiku_dirent& dirent)
{
    return HAIKU_POSIX_ENOSYS;
}

status_t HostfsDevice::RewindDir(VfsDir& dir)
{
    return HAIKU_POSIX_ENOSYS;
}