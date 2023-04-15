#include <cstring>

#include "bindfs.h"
#include "system.h"

haiku_ino_t BindfsDevice::_Hash(uint64_t hostDev, uint64_t hostIno)
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

BindfsDevice::BindfsDevice(const std::filesystem::path& root, const std::filesystem::path& boundRoot,
    uint32 mountFlags)
    : VfsDevice(root, mountFlags), _boundRoot(boundRoot)
{
    _info.flags = B_FS_IS_PERSISTENT | B_FS_IS_SHARED | B_FS_HAS_ATTR;
    _info.block_size = 0;
    _info.io_size = 0;
    _info.total_blocks = 0;
    _info.free_blocks = 0;
    _info.total_nodes = 0;
    _info.free_nodes = 0;
    strncpy(_info.volume_name, root.filename().c_str(), sizeof(_info.volume_name));
    strncpy(_info.fsh_name, "bindfs", sizeof(_info.fsh_name));
}

status_t BindfsDevice::_TransformPath(std::filesystem::path& path)
{
    assert(path.is_absolute());
    assert(path.lexically_normal() == path);

    auto relativePath = path.lexically_relative(_root);
    if (relativePath.empty())
    {
        return B_ENTRY_NOT_FOUND;
    }
    if (relativePath.begin()->string() == "..")
    {
        return B_ENTRY_NOT_FOUND;
    }

    if (relativePath.begin()->string() != ".")
    {
        path = _boundRoot / relativePath;
    }
    else
    {
        path = _boundRoot;
    }

    return B_OK;
}

status_t BindfsDevice::GetPath(std::filesystem::path& path, bool& isSymlink)
{
    auto status = _TransformPath(path);
    if (status != B_OK)
    {
        return status;
    }

    auto& vfsService = System::GetInstance().GetVfsService();
    return vfsService.GetPath(path, isSymlink);
}

status_t BindfsDevice::GetAttrPath(std::filesystem::path& path, const std::string& name,
    uint32 type, bool createNew, bool& isSymlink)
{
    auto status = _TransformPath(path);
    if (status != B_OK)
    {
        return status;
    }

    auto& vfsService = System::GetInstance().GetVfsService();
    return vfsService.GetAttrPath(path, name, type, createNew, isSymlink);
}

status_t BindfsDevice::RealPath(std::filesystem::path& path, bool& isSymlink)
{
    auto status = _TransformPath(path);
    if (status != B_OK)
    {
        return status;
    }

    auto& vfsService = System::GetInstance().GetVfsService();
    status = vfsService.RealPath(path);

    auto relativePath = path.lexically_relative(_boundRoot);
    if (relativePath.empty() || relativePath == ".")
    {
        path = _root;
    }
    else
    {
        path = _root / relativePath;
    }

    isSymlink = false;

    return status;
}

status_t BindfsDevice::ReadStat(std::filesystem::path& path, haiku_stat& stat, bool& isSymlink)
{
    auto status = _TransformPath(path);
    if (status != B_OK)
    {
        return status;
    }

    auto& vfsService = System::GetInstance().GetVfsService();
    status = vfsService.ReadStat(path, stat, isSymlink);

    if (status == B_OK)
    {
        stat.st_ino = _Hash(stat.st_dev, stat.st_ino);
        stat.st_dev = _info.dev;
    }

    return status;
}

status_t BindfsDevice::WriteStat(std::filesystem::path& path, const haiku_stat& stat,
    int statMask, bool& isSymlink)
{
    auto status = _TransformPath(path);
    if (status != B_OK)
    {
        return status;
    }

    auto& vfsService = System::GetInstance().GetVfsService();
    return vfsService.WriteStat(path, stat, statMask, isSymlink);
}

status_t BindfsDevice::StatAttr(const std::filesystem::path& path, const std::string& name,
    haiku_attr_info& info)
{
    auto boundPath = path;
    auto status = _TransformPath(boundPath);
    if (status != B_OK)
    {
        return status;
    }

    auto& vfsService = System::GetInstance().GetVfsService();
    return vfsService.StatAttr(boundPath, name, info);
}

haiku_ssize_t BindfsDevice::ReadAttr(const std::filesystem::path& path, const std::string& name, size_t pos,
    void* buffer, size_t size)
{
    auto boundPath = path;
    auto status = _TransformPath(boundPath);
    if (status != B_OK)
    {
        return status;
    }

    auto& vfsService = System::GetInstance().GetVfsService();
    return vfsService.ReadAttr(boundPath, name, pos, buffer, size);
}

haiku_ssize_t BindfsDevice::WriteAttr(const std::filesystem::path& path, const std::string& name, uint32 type,
    size_t pos, const void* buffer, size_t size)
{
    auto boundPath = path;
    auto status = _TransformPath(boundPath);
    if (status != B_OK)
    {
        return status;
    }

    auto& vfsService = System::GetInstance().GetVfsService();
    return vfsService.WriteAttr(boundPath, name, type, pos, buffer, size);
}

haiku_ssize_t BindfsDevice::RemoveAttr(const std::filesystem::path& path, const std::string& name)
{
    auto boundPath = path;
    auto status = _TransformPath(boundPath);
    if (status != B_OK)
    {
        return status;
    }

    auto& vfsService = System::GetInstance().GetVfsService();
    return vfsService.RemoveAttr(boundPath, name);
}

status_t BindfsDevice::TransformDirent(const std::filesystem::path& path, haiku_dirent& dirent)
{
    auto boundPath = path;
    auto status = _TransformPath(boundPath);
    if (status != B_OK)
    {
        return status;
    }

    auto& vfsService = System::GetInstance().GetVfsService();
    status = vfsService.TransformDirent(boundPath, dirent);

    if (status == B_OK)
    {
        dirent.d_ino = _Hash(dirent.d_dev, dirent.d_ino);
        dirent.d_dev = _info.dev;
    }

    return status;
}

status_t BindfsDevice::Mount(const std::filesystem::path& path,
    const std::filesystem::path& device, uint32 flags, const std::string& args,
    std::shared_ptr<VfsDevice>& output)
{
    std::filesystem::path source;

    if (args.empty())
    {
        return B_BAD_VALUE;
    }

    if (args.compare(0, sizeof("source") - 1, "source") != 0)
    {
        return B_BAD_VALUE;
    }

    auto sourceStr = args.substr(sizeof("source"));

    size_t first = sourceStr.find_first_not_of(' ');
    if (first != std::string::npos)
    {
        sourceStr = sourceStr.substr(first);
    }

    source = sourceStr;

    auto& vfsService = System::GetInstance().GetVfsService();
    auto status = vfsService.RealPath(source);

    if (status != B_OK)
    {
        return status;
    }

    output = std::make_shared<BindfsDevice>(path, source, flags);

    return B_OK;
}