#include "haiku_errors.h"
#include "hostfs.h"
#include "server_errno.h"
#include "server_native.h"
#include "system.h"

HostfsDevice::HostfsDevice(const std::filesystem::path& root, const std::filesystem::path& hostRoot)
    : VfsDevice(root), _hostRoot(hostRoot)
{

}

haiku_ino_t HostfsDevice::_Hash(uint64_t hostDev, uint64_t hostIno)
{
    haiku_ino_t result = 0;
    char* data = (char*)&hostDev;
    for (int i = 0; i < sizeof(uint64_t); ++i)
    {
        result = *data++ + (result << 6) + (result << 16) - result;
    }
    data = (char*)&hostIno;
    for (int i = 0; i < sizeof(uint64_t); ++i)
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

    if (ec)
    {
        std::vector<std::filesystem::path> parts(relativePath.begin(), relativePath.end());

        hostPath = _hostRoot;
        path = _root;

        auto it = parts.begin();
        for (; it != parts.end(); ++it)
        {
            hostPath /= *it;
            path /= *it;
            status = std::filesystem::symlink_status(hostPath, ec);
            if (ec)
            {
                return CppToB(ec);
            }
            if (std::filesystem::is_symlink(status))
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
        }
        return B_ENTRY_NOT_FOUND;
    }
    else
    {
        if (isSymlink)
        {
            isSymlink = std::filesystem::is_symlink(status);
            if (isSymlink)
            {
                std::error_code ec;
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
                path = path.lexically_normal();
                return B_OK;
            }
        }

        path = hostPath;
        return B_OK;
    }
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

status_t HostfsDevice::OpenDir(std::filesystem::path& path, VfsDir& dir, bool& isSymlink)
{
    assert(path.is_absolute());
    assert(path.lexically_normal() == path);

    status_t status = GetPath(path, isSymlink);

    if (isSymlink || status != B_OK)
    {
        return status;
    }

    auto relativePath = path.lexically_relative(_hostRoot);
    dir.path = _root / relativePath;

    dir.cookie = 0;
    dir.device = std::weak_ptr<VfsDevice>(shared_from_this());
    dir.hostPath = path;

    std::error_code ec;
    dir.dir = std::filesystem::directory_iterator(path, ec);

    if (ec)
    {
        return CppToB(ec);
    }

    struct haiku_stat stat;
    status = server_read_stat(path, stat);

    if (status != B_OK)
    {
        return status;
    }

    dir.ino = _Hash(stat.st_dev, stat.st_ino);
    dir.dev = _info.dev;

    return B_OK;
}

status_t HostfsDevice::ReadDir(VfsDir& dir, haiku_dirent& dirent)
{
    while (dir.dir != std::filesystem::directory_iterator())
    {
        auto& entry = *dir.dir;

        if (_IsBlacklisted(entry))
        {
            ++dir.dir;
            continue;
        }

        size_t maxFileNameLength = dirent.d_reclen - sizeof(haiku_dirent);
        std::string fileName = entry.path().filename().string();
        if (fileName.length() >= maxFileNameLength)
        {
            return B_BUFFER_OVERFLOW;
        }

        std::shared_ptr<VfsDevice> device;
        {
            auto& vfsService = System::GetInstance().GetVfsService();
            auto lock = vfsService.Lock();
            device = vfsService.GetDevice(entry.path()).lock();
        }

        if (!device)
        {
            device = dir.device.lock();
        }

        if (!device)
        {
            return B_ENTRY_NOT_FOUND;
        }

        struct haiku_stat stat;
        status_t status = server_read_stat(entry.path(), stat);

        if (status != B_OK)
        {
            return status;
        }

        dirent.d_ino = _Hash(stat.st_dev, stat.st_ino);
        dirent.d_dev = device->GetInfo().dev;
        dirent.d_pdev = dir.dev;
        dirent.d_pino = dir.ino;
        dirent.d_reclen = sizeof(haiku_dirent) + fileName.length();
        fileName.copy(dirent.d_name, fileName.length());
        dirent.d_name[fileName.length()] = '\0';

        ++dir.dir;
        return B_OK;
    }

    return B_ENTRY_NOT_FOUND;
}

status_t HostfsDevice::RewindDir(VfsDir& dir)
{
    dir.cookie = 0;
    dir.dir = std::filesystem::directory_iterator(dir.hostPath);
    return B_OK;
}