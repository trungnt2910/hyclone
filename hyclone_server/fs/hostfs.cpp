#include <cstring>

#include "haiku_errors.h"
#include "hostfs.h"
#include "server_errno.h"
#include "server_native.h"
#include "system.h"

HostfsDevice::HostfsDevice(const std::filesystem::path& root,
    const std::filesystem::path& hostRoot, uint32 mountFlags)
    : VfsDevice(root, mountFlags), _hostRoot(hostRoot)
{

}

haiku_ino_t HostfsDevice::_Hash(uint64_t hostDev, uint64_t hostIno)
{
    haiku_ino_t result = 0;
    char* data = (char*)&hostDev;
    for (size_t i = 0; i < sizeof(uint64_t); ++i)
    {
        result = *data++ + (result << 6) + (result << 16) - result;
    }
    data = (char*)&hostIno;
    for (size_t i = 0; i < sizeof(uint64_t); ++i)
    {
        result = *data++ + (result << 6) + (result << 16) - result;
    }
    return result;
}

status_t HostfsDevice::GetPath(std::filesystem::path& path, bool& isSymlink)
{
    assert(path.is_absolute());
    assert(path.lexically_normal() == path);

    auto relativePath = path.lexically_relative(_root);
    if (relativePath.empty())
    {
        return B_ENTRY_NOT_FOUND;
    }

    auto hostPath = _hostRoot;

    if (relativePath != ".")
    {
        hostPath /= relativePath;
    }

    if (_IsBlacklisted(hostPath))
    {
        return B_ENTRY_NOT_FOUND;
    }

    std::error_code ec;
    std::filesystem::file_status status
        = std::filesystem::symlink_status(hostPath, ec);

    if (ec && ec.value() != (int)std::errc::no_such_file_or_directory)
    {
        return CppToB(ec);
    }

    std::vector<std::filesystem::path> parts(relativePath.begin(), relativePath.end());

    auto originalHostPath = std::move(hostPath);
    hostPath = _hostRoot;
    path = _root;

    for (auto it = parts.begin(); it != parts.end(); ++it)
    {
        hostPath /= *it;
        path /= *it;
        status = std::filesystem::symlink_status(hostPath, ec);
        if (ec)
        {
            if (ec.value() != (int)std::errc::no_such_file_or_directory)
            {
                return CppToB(ec);
            }
            isSymlink = false;
            path = originalHostPath;
            return B_ENTRY_NOT_FOUND;
        }
        if (std::filesystem::is_symlink(status))
        {
            bool pathIncomplete = it != parts.end() - 1;

            if (isSymlink || pathIncomplete)
            {
                auto symlinkPath = std::filesystem::read_symlink(hostPath, ec);
                if (ec)
                {
                    return CppToB(ec);
                }

                if (symlinkPath.is_absolute())
                {
                    path = symlinkPath;
                }
                else
                {
                    path = path.parent_path() / symlinkPath;
                }

                while (++it != parts.end())
                {
                    path /= *it;
                }

                path = path.lexically_normal();

                isSymlink = true;
                return B_OK;
            }

            break;
        }
    }
    isSymlink = false;
    path = originalHostPath;
    return ec ? B_ENTRY_NOT_FOUND : B_OK;
}

status_t HostfsDevice::RealPath(std::filesystem::path& path, bool& isSymlink)
{
    std::filesystem::path originalPath = path;
    status_t status = GetPath(path, isSymlink);
    if (!isSymlink)
    {
        auto relativePath = path.lexically_relative(_hostRoot);
        if (relativePath.empty() || relativePath == ".")
        {
            path = _root;
        }
        else
        {
            path = _root / relativePath;
        }
    }

    return status;
}

status_t HostfsDevice::ReadStat(std::filesystem::path& path, haiku_stat& stat, bool& isSymlink)
{
    assert(path.is_absolute());
    assert(path.lexically_normal() == path);

    status_t status = GetPath(path, isSymlink);

    if (isSymlink || status != B_OK)
    {
        return status;
    }

    status = server_read_stat(path, stat);

    if (status == B_OK)
    {
        stat.st_ino = _Hash(stat.st_dev, stat.st_ino);
        stat.st_dev = _info.dev;
    }

    return status;
}

status_t HostfsDevice::WriteStat(std::filesystem::path& path, const haiku_stat& stat,
    int statMask, bool& isSymlink)
{
    assert(path.is_absolute());
    assert(path.lexically_normal() == path);

    status_t status = GetPath(path, isSymlink);

    if (isSymlink || status != B_OK)
    {
        return status;
    }

    return server_write_stat(path, stat, statMask);
}

status_t HostfsDevice::TransformDirent(const std::filesystem::path& path, haiku_dirent& dirent)
{
    assert(path.is_absolute());
    assert(path.lexically_normal() == path);

    auto filePath = (path / dirent.d_name).lexically_normal();

    auto& vfsService = System::GetInstance().GetVfsService();
    auto lock = vfsService.Lock();

    haiku_stat stat;
    status_t status = vfsService.ReadStat(filePath, stat, false);

    if (status != B_OK)
    {
        return status;
    }

    dirent.d_dev = stat.st_dev;
    dirent.d_ino = stat.st_ino;

    if (strcmp(dirent.d_name, ".") && strcmp(dirent.d_name, ".."))
    {
        vfsService.RegisterEntryRef(EntryRef(stat.st_dev, stat.st_ino), filePath);
    }

    return dirent.d_reclen;
}