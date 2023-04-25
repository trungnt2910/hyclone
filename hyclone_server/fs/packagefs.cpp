#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

#include <hpkgvfs/Entry.h>
#include <hpkgvfs/Package.h>

#include "fs/packagefs.h"
#include "server_errno.h"
#include "server_filesystem.h"
#include "server_native.h"
#include "system.h"

std::filesystem::path PackagefsDevice::_relativeAttributesPath = std::filesystem::path(".hyclone.pkgfsattrs");
std::filesystem::path PackagefsDevice::_relativeInstalledPackagesPath = std::filesystem::path(".hpkgvfsPackages");
std::filesystem::path PackagefsDevice::_relativePackagesPath = std::filesystem::path("packages");

class PackagefsEntryWriter : public HpkgVfs::EntryWriter
{
private:
    PackagefsDevice& _device;
public:
    PackagefsEntryWriter(PackagefsDevice& device)
        : _device(device)
    {
    }

    virtual void WriteExtendedAttributes(const std::filesystem::path& path,
        const std::vector<HpkgVfs::ExtendedAttribute>& attributes) override
    {
        auto relativePath = path.lexically_relative(_device._hostRoot);

        for (const auto& attr : attributes)
        {
            _device.WriteAttr(_device._root / relativePath, attr.Name, attr.Type, 0, attr.Data.data(), attr.Data.size());
        }
    }
};

PackagefsDevice::PackagefsDevice(const std::filesystem::path& root,
    const std::filesystem::path& hostRoot, PackageFSMountType mountType, uint32 mountFlags)
    : HostfsDevice(root, hostRoot, mountFlags), _mountType(mountType)
{
    auto permissions = std::filesystem::status(hostRoot).permissions();
    std::filesystem::permissions(hostRoot, permissions | std::filesystem::perms::owner_write);
    std::filesystem::create_directories(hostRoot / _relativeAttributesPath);
    std::filesystem::create_directories(hostRoot / _relativeInstalledPackagesPath);
    std::filesystem::create_directories(hostRoot / _relativePackagesPath);
    std::filesystem::permissions(hostRoot, permissions);

    using namespace HpkgVfs;

    std::filesystem::path packagesPath = hostRoot / _relativePackagesPath;
    std::filesystem::path installedPackagesPath = hostRoot / _relativeInstalledPackagesPath;

    std::shared_ptr<Entry> system = Entry::CreatePackageFsRootEntry(_root.filename().string());

    std::unordered_map<std::string, haiku_timespec> installedPackages;
    std::unordered_map<std::string, haiku_timespec> enabledPackages;

    if (std::filesystem::exists(installedPackagesPath))
    {
        for (const auto& file : std::filesystem::directory_iterator(installedPackagesPath))
        {
            if (file.path().extension() != ".hpkg")
            {
                continue;
            }

            std::ifstream fin(file.path());
            if (!fin.is_open())
            {
                continue;
            }

            Package package(file.path().string());
            struct haiku_stat st;
            if (server_read_stat(file.path(), st) != B_OK)
            {
                continue;
            }
            installedPackages.emplace(file.path().stem(), st.st_mtim);
            std::cerr << "Preinstalled package: " << file.path().stem() << std::endl;
            std::shared_ptr<Entry> entry = package.GetRootEntry(/*dropData*/ true);
            system->Merge(entry);
        }
    }

    std::vector<std::pair<std::filesystem::path, std::string>> toRename;

    for (const auto& file : std::filesystem::directory_iterator(packagesPath))
    {
        auto currentPath = root / _relativePackagesPath / file.path().filename();
        auto originalPath = file.path();

        if (currentPath.extension() != ".hpkg")
        {
            continue;
        }

        if (file.symlink_status().type() == std::filesystem::file_type::directory)
        {
            continue;
        }

        status_t status = _ResolvePackagePath(currentPath);

        if (status != B_OK)
        {
            std::cerr << "Failed to resolve symlink: " << originalPath << std::endl;
            std::cerr << "Skipping package: " << file.path().filename() << std::endl;
            continue;
        }

        std::string name;
        std::string fileName;

        {
            Package package(currentPath.string());
            name = package.GetId();
            fileName = name + ".hpkg";
        }

        if (originalPath.filename() != fileName)
        {
            toRename.emplace_back(originalPath, fileName);
        }

        // Package names might not match the ID.
        // We must read the data from the package.
        struct haiku_stat st;
        if (server_read_stat(currentPath, st) != B_OK)
        {
            continue;
        }
        enabledPackages.emplace(name, st.st_mtim);
    }

    for (const auto& pair : toRename)
    {
        const std::filesystem::path& oldName = pair.first;
        auto newName = oldName.parent_path() / pair.second;
        std::cerr << "Renaming " << oldName << " to " << newName << std::endl;
        std::filesystem::rename(oldName, newName);
    }

    PackagefsEntryWriter writer(*this);

    for (const auto& kvp: installedPackages)
    {
        auto it = enabledPackages.find(kvp.first);
        if (it == enabledPackages.end())
        {
            std::cerr << "Uninstalling package: " << kvp.first << std::endl;
            system->RemovePackage(kvp.first);
            system->WriteToDisk(hostRoot.parent_path(), writer);
        }
        else if (std::make_pair(it->second.tv_sec, it->second.tv_nsec) >
            std::make_pair(kvp.second.tv_sec, kvp.second.tv_nsec))
        {
            std::cerr << "Updating package: " << kvp.first << " (later timestamp detected)" << std::endl;
            system->RemovePackage(kvp.first);
            auto package = _GetPackageInPackagesDirectory(it->first);
            if (!package)
            {
                std::cerr << "Failed to update package: " << kvp.first << std::endl;
                continue;
            }
            std::shared_ptr<Entry> entry = package->GetRootEntry();
            system->Merge(entry);
            system->WriteToDisk(hostRoot.parent_path(), writer);
            system->Drop();
        }
        else
        {
            std::cerr << "Keeping installed package: " << kvp.first << std::endl;
        }

        if (it != enabledPackages.end())
        {
            enabledPackages.erase(it);
        }
    }

    for (const auto& pkg: enabledPackages)
    {
        std::cerr << "Installing package: " << pkg.first << std::endl;
        auto package = _GetPackageInPackagesDirectory(pkg.first);
        if (!package)
        {
            std::cerr << "Failed to install package: " << pkg.first << std::endl;
            continue;
        }
        std::shared_ptr<Entry> entry = package->GetRootEntry();
        system->Merge(entry);
        system->WriteToDisk(hostRoot.parent_path(), writer);
        system->Drop();
    }

    auto librootPath = _hostRoot / "lib" / "libroot.so";
    if (std::filesystem::exists(librootPath))
    {
        server_replace_libroot(librootPath);
    }

    std::cerr << "Done extracting packagefs at " << _root << std::endl;

    _CleanupAttributes();

    _info.flags = B_FS_IS_PERSISTENT | B_FS_IS_SHARED | B_FS_HAS_ATTR;
    _info.block_size = 0;
    _info.io_size = 0;
    _info.total_blocks = 0;
    _info.free_blocks = 0;
    _info.total_nodes = 0;
    _info.free_nodes = 0;
    strncpy(_info.volume_name, hostRoot.filename().c_str(), sizeof(_info.volume_name));
    strncpy(_info.fsh_name, "packagefs", sizeof(_info.fsh_name));

    server_fill_fs_info(hostRoot, &_info);
}

std::filesystem::path PackagefsDevice::_GetAttrPathInternal(const std::filesystem::path& path, const std::string& name)
{
    std::filesystem::path result = _relativeAttributesPath;

    for (const auto& part : path)
    {
        result /= (char)PACKAGEFS_ATTREMU_FILE + part.string();
    }

    if (name.empty())
    {
        return result;
    }

    std::string resultName(1, (char)PACKAGEFS_ATTREMU_ATTR);

    for (const auto& c : name)
    {
        switch (c)
        {
            case '/':
                resultName += "_s";
            break;
            case '.':
                resultName += "_d";
            break;
            case '_':
                resultName += "_u";
            default:
                resultName += c;
                break;
        }
    }

    result /= resultName;

    return result;
}

std::string PackagefsDevice::_UnescapeAttrName(const std::string& name)
{
    std::string result;

    for (size_t i = 1; i < name.size(); ++i)
    {
        if (name[i] == '_')
        {
            if (i + 1 >= name.size())
            {
                result += '_';
                break;
            }

            switch (name[i + 1])
            {
                case 's':
                    result += '/';
                    break;
                case 'd':
                    result += '.';
                    break;
                case 'u':
                    result += '_';
                    break;
                default:
                    result += '_';
                    result += name[i + 1];
                    break;
            }

            ++i;
        }
        else
        {
            result += name[i];
        }
    }

    return result;
}

status_t PackagefsDevice::_ResolvePackagePath(std::filesystem::path& path)
{
    auto& vfsService = System::GetInstance().GetVfsService();
    auto lock = vfsService.Lock();

    return vfsService.GetPath(path);
}

void PackagefsDevice::_CleanupAttributes()
{
    for (auto it = std::filesystem::recursive_directory_iterator(_hostRoot / _relativeAttributesPath);
        it != std::filesystem::recursive_directory_iterator(); )
    {
        auto& entry = *it;

        if (!entry.is_directory())
        {
            ++it;
            continue;
        }

        auto attrPath = entry.path();
        auto relativePath = attrPath.lexically_relative(_hostRoot / _relativeAttributesPath);
        auto filePath = _hostRoot;

        for (const auto& part : relativePath)
        {
            filePath /= part.string().substr(1);
        }

        if (!std::filesystem::exists(filePath))
        {
            it.disable_recursion_pending();
            ++it;
            std::cerr << "Removing stale attribute directory: " << attrPath << " as " << filePath << " does not exist" << std::endl;
            std::error_code ec;
            std::filesystem::remove_all(attrPath, ec);
            if (ec)
            {
                std::cerr << "Failed to remove stale attribute directory: " << attrPath << std::endl;
            }
        }
        else
        {
            ++it;
        }
    }
}

std::shared_ptr<HpkgVfs::Package> PackagefsDevice::_GetPackageInPackagesDirectory(const std::string& name)
{
    auto packagePath = _root / _relativePackagesPath / (name + ".hpkg");
    status_t status = _ResolvePackagePath(packagePath);
    if (status != B_OK)
    {
        std::cerr << "Failed to resolve package path: " << name << " " << status << std::endl;
        return std::shared_ptr<HpkgVfs::Package>();
    }
    return std::make_shared<HpkgVfs::Package>(packagePath);
}

bool PackagefsDevice::_IsBlacklisted(const std::filesystem::path& hostPath) const
{
    if (hostPath.lexically_relative(_hostRoot) == _relativeInstalledPackagesPath)
    {
        return true;
    }
    if (hostPath.lexically_relative(_hostRoot) == _relativeAttributesPath)
    {
        return true;
    }
    return false;
}

bool PackagefsDevice::_IsBlacklisted(const std::filesystem::directory_entry& entry) const
{
    return _IsBlacklisted(entry.path());
}

status_t PackagefsDevice::GetAttrPath(std::filesystem::path& path, const std::string& name,
    uint32 type, bool createNew, bool& isSymlink)
{
    status_t status = GetPath(path, isSymlink);

    if (isSymlink || status != B_OK)
    {
        return status;
    }

    auto relativePath = path.lexically_relative(_hostRoot);
    auto attrRelativePath = _GetAttrPathInternal(relativePath, name);
    auto attrHostPath = _hostRoot / attrRelativePath;

    if (name.empty())
    {
        std::filesystem::create_directories(attrHostPath);
    }
    else if (createNew && !std::filesystem::exists(attrHostPath))
    {
        std::filesystem::create_directories(attrHostPath.parent_path());
        auto attrTypeName = attrHostPath.filename().string();
        attrTypeName[0] = PACKAGEFS_ATTREMU_TYPE;
        std::ofstream fout(attrHostPath.parent_path() / attrTypeName);
        fout.write((char*)&type, sizeof(type));
    }

    path = _root / attrRelativePath;

    return B_OK;
}

status_t PackagefsDevice::StatAttr(const std::filesystem::path& path, const std::string& name,
    haiku_attr_info& info)
{
    auto relativePath = path.lexically_relative(_root);
    auto attrRelativePath = _GetAttrPathInternal(relativePath, name);

    auto attrHostPath = _hostRoot / attrRelativePath;

    if (!std::filesystem::exists(attrHostPath))
    {
        return B_ENTRY_NOT_FOUND;
    }

    auto attrTypeName = attrHostPath.filename().string();
    attrTypeName[0] = PACKAGEFS_ATTREMU_TYPE;
    std::ifstream fin(attrHostPath.parent_path() / attrTypeName);
    fin.read((char*)&info.type, sizeof(info.type));

    fin.close();
    fin.open(attrHostPath, std::ios::binary | std::ios::ate);
    info.size = fin.tellg();

    return B_OK;
}

status_t PackagefsDevice::TransformDirent(const std::filesystem::path& path, haiku_dirent& dirent)
{
    auto relativePath = path.lexically_relative(_root);

    auto relativeAttributesPathString = _relativeAttributesPath.string();

    if (relativePath.string().compare(0, relativeAttributesPathString.size(), relativeAttributesPathString) != 0)
    {
        return HostfsDevice::TransformDirent(path, dirent);
    }

    dirent.d_ino = _Hash(dirent.d_dev, dirent.d_ino);
    dirent.d_dev = _info.dev;

    if (strcmp(dirent.d_name, ".") == 0 || strcmp(dirent.d_name, "..") == 0)
    {
        return B_BAD_VALUE;
    }

    if (dirent.d_name[0] != PACKAGEFS_ATTREMU_ATTR)
    {
        return B_BAD_VALUE;
    }

    std::string name = dirent.d_name;
    std::string unescapedName = _UnescapeAttrName(name);

    dirent.d_reclen -= name.size() - unescapedName.size();
    strcpy(dirent.d_name, unescapedName.c_str());

    return dirent.d_reclen;
}

status_t PackagefsDevice::Ioctl(const std::filesystem::path& path, unsigned int cmd, void* addr, void* buffer, size_t size)
{
    switch (cmd)
    {
        case PACKAGE_FS_OPERATION_GET_VOLUME_INFO:
        {
            if (size < sizeof(PackageFSVolumeInfo))
            {
                return B_BAD_VALUE;
            }

            auto volumeInfo = (PackageFSVolumeInfo*)buffer;
            volumeInfo->mountType = _mountType;

            auto& vfsService = System::GetInstance().GetVfsService();

            haiku_stat st;
            status_t status = B_OK;

            switch (_mountType)
            {
                case PACKAGE_FS_MOUNT_TYPE_SYSTEM:
                    status = vfsService.ReadStat(_root.parent_path(), st, false);
                break;
                case PACKAGE_FS_MOUNT_TYPE_HOME:
                    status = vfsService.ReadStat(_root.parent_path().parent_path(), st, false);
                break;
                case PACKAGE_FS_MOUNT_TYPE_CUSTOM:
                    st.st_dev = -1;
                    st.st_ino = 0;
                break;
                default:
                    std::cerr << "Unknown mount type: " << _mountType << std::endl;
                    return B_BAD_VALUE;
            }

            if (status != B_OK)
            {
                return status;
            }

            volumeInfo->rootDeviceID = st.st_dev;
            volumeInfo->rootDirectoryID = st.st_ino;

            status = vfsService.ReadStat(_root / _relativePackagesPath, st, false);
            if (status != B_OK)
            {
                return status;
            }

            volumeInfo->packagesDirectoryCount = 1;
            volumeInfo->packagesDirectoryInfos[0].deviceID = st.st_dev;
            volumeInfo->packagesDirectoryInfos[0].nodeID = st.st_ino;

            vfsService.RegisterEntryRef(EntryRef(st.st_dev, st.st_ino), _root / _relativePackagesPath);

            return B_OK;
        }
        case PACKAGE_FS_OPERATION_GET_PACKAGE_INFOS:
        {
            if (size < sizeof(PackageFSGetPackageInfosRequest))
            {
                return B_BAD_VALUE;
            }

            auto request = (PackageFSGetPackageInfosRequest*)buffer;

            std::vector<std::filesystem::directory_entry> packages(
                std::filesystem::directory_iterator(_hostRoot / _relativeInstalledPackagesPath),
                std::filesystem::directory_iterator());

            addr_t bufferEnd = (addr_t)buffer + size;
            uint32 packageCount = packages.size();
            char* nameBuffer = (char*)(request->infos + packageCount);

            auto& vfsService = System::GetInstance().GetVfsService();

            haiku_stat dirst;
            status_t status = vfsService.ReadStat(_root / _relativePackagesPath, dirst, false);
            if (status != B_OK)
            {
                return status;
            }

            uint32 packageIndex = 0;
            for (; packageIndex < packageCount; ++packageIndex)
            {
                auto& package = packages[packageIndex];
                PackageFSPackageInfo* info = request->infos + packageIndex;

                if ((addr_t)(info + 1) <= bufferEnd)
                {
                    info->directoryDeviceID = dirst.st_dev;
                    info->directoryNodeID = dirst.st_ino;

                    auto installedPackagePath = _root / _relativePackagesPath / package.path().filename();

                    haiku_stat st;
                    status = vfsService.ReadStat(installedPackagePath, st, true);

                    if (status != B_OK)
                    {
                        assert(server_read_stat(package.path(), st) == B_OK);
                    }

                    info->packageDeviceID = st.st_dev;
                    info->packageNodeID = st.st_ino;

                    if (status == B_OK)
                    {
                        vfsService.RegisterEntryRef(EntryRef(st.st_dev, st.st_ino), installedPackagePath);
                    }
                }

                auto name = package.path().filename().string();
                size_t nameSize = name.length() + 1;
                char* nameEnd = nameBuffer + nameSize;

                if ((addr_t)nameEnd <= bufferEnd)
                {
                    memcpy(nameBuffer, name.c_str(), nameSize);
                    info->name = (char *)((addr_t)addr + ((addr_t)nameBuffer - (addr_t)buffer));
                }

                nameBuffer = nameEnd;
            }

            request->bufferSize = (addr_t)nameBuffer - (addr_t)request;
            request->packageCount = packageCount;
            return B_OK;
        }
        case PACKAGE_FS_OPERATION_CHANGE_ACTIVATION:
        {
            PackageFSActivationChangeRequest* request = (PackageFSActivationChangeRequest*)buffer;
            uint32 itemCount = request->itemCount;
            addr_t requestEnd = (addr_t)buffer + size;

            if (&request->items[itemCount] > (PackageFSActivationChangeItem*)requestEnd)
            {
                return B_BAD_VALUE;
            }

            addr_t nameDelta = (addr_t)request - (addr_t)addr;

            for (uint32 i = 0; i < itemCount; ++i)
            {
                PackageFSActivationChangeItem& item = request->items[i];
                item.name = (char*)((addr_t)item.name + nameDelta);

                if (item.name < (char*)buffer || item.name >= (char*)requestEnd)
                {
                    return B_BAD_VALUE;
                }

                size_t maxNameSize = requestEnd - (addr_t)item.name;
                if (strnlen(item.name, maxNameSize) >= maxNameSize)
                {
                    return B_BAD_VALUE;
                }
            }

            std::unique_lock lock(_updateMutex);

            using namespace HpkgVfs;

            std::filesystem::path emulatedPackagesPath = _root / _relativePackagesPath;
            std::filesystem::path installedPackagesPath = _hostRoot / _relativeInstalledPackagesPath;

            std::shared_ptr<Entry> system = Entry::CreatePackageFsRootEntry(_root.filename().string());

            std::unordered_set<std::string> installedPackages;

            for (const auto& file : std::filesystem::directory_iterator(installedPackagesPath))
            {
                if (file.path().extension() != ".hpkg")
                {
                    continue;
                }

                std::ifstream fin(file.path());
                if (!fin.is_open())
                {
                    continue;
                }

                Package package(file.path().string());
                installedPackages.emplace(file.path().stem());
                std::cerr << "Preinstalled package: " << file.path().stem() << std::endl;
                std::shared_ptr<Entry> entry = package.GetRootEntry(/*dropData*/ true);
                system->Merge(entry);
            }

            PackagefsEntryWriter writer(*this);

            for (uint32 i = 0; i < itemCount; ++i)
            {
                PackageFSActivationChangeItem& item = request->items[i];
                auto name = std::filesystem::path(item.name).stem();

                auto it = installedPackages.find(name);

                if (it == installedPackages.end())
                {
                    if (item.type != PACKAGE_FS_ACTIVATE_PACKAGE)
                    {
                        std::cerr << "Package not found: " << name << std::endl;
                        continue;
                    }
                    std::cerr << "Installing package: " << name << std::endl;
                    auto package = _GetPackageInPackagesDirectory(name.string());
                    if (!package)
                    {
                        std::cerr << "Package not found: " << name << std::endl;
                        continue;
                    }
                    std::shared_ptr<Entry> entry = package->GetRootEntry();
                    system->Merge(entry);
                    system->WriteToDisk(_hostRoot.parent_path(), writer);
                    system->Drop();
                }
                else
                {
                    if (item.type == PACKAGE_FS_DEACTIVATE_PACKAGE)
                    {
                        std::cerr << "Uninstalling package: " << *it << std::endl;
                        system->RemovePackage(*it);
                        system->WriteToDisk(_hostRoot.parent_path(), writer);
                    }
                    else if (item.type == PACKAGE_FS_REACTIVATE_PACKAGE)
                    {
                        auto package = _GetPackageInPackagesDirectory(name.string());
                        if (!package)
                        {
                            std::cerr << "Package not found: " << name << std::endl;
                            continue;
                        }
                        system->RemovePackage(*it);
                        std::shared_ptr<Entry> entry = package->GetRootEntry();
                        system->Merge(entry);
                        system->WriteToDisk(_hostRoot.parent_path(), writer);
                        system->Drop();
                    }
                    else if (item.type == PACKAGE_FS_ACTIVATE_PACKAGE)
                    {
                        std::cerr << "Package already installed: " << *it << std::endl;
                    }
                    else
                    {
                        std::cerr << "Unknown activation change type: " << item.type << std::endl;
                    }
                }
            }

            auto librootPath = _hostRoot / "lib" / "libroot.so";
            if (std::filesystem::exists(librootPath))
            {
                server_replace_libroot(librootPath);
            }

            std::cerr << "Done updating packagefs at " << _root << std::endl;

            _CleanupAttributes();

            return B_OK;
        }
        default:
        {
            return HostfsDevice::Ioctl(path, cmd, addr, buffer, size);
        }
    }
}

haiku_ssize_t PackagefsDevice::ReadAttr(const std::filesystem::path& path, const std::string& name, size_t pos,
    void* buffer, size_t size)
{
    auto relativePath = path.lexically_relative(_root);
    auto attrRelativePath = _GetAttrPathInternal(relativePath, name);

    auto attrHostPath = _hostRoot / attrRelativePath;

    std::ifstream fin(attrHostPath, std::ios::binary);
    if (!fin.is_open())
    {
        return B_ENTRY_NOT_FOUND;
    }

    fin.seekg(pos, std::ios::beg);
    fin.read((char*)buffer, size);

    return fin.gcount();
}

haiku_ssize_t PackagefsDevice::WriteAttr(const std::filesystem::path& path, const std::string& name, uint32 type,
    size_t pos, const void* buffer, size_t size)
{
    auto relativePath = path.lexically_relative(_root);
    auto attrRelativePath = _GetAttrPathInternal(relativePath, name);

    auto attrHostPath = _hostRoot / attrRelativePath;

    auto attrTypeName = attrHostPath.filename().string();
    attrTypeName[0] = PACKAGEFS_ATTREMU_TYPE;
    auto attrTypeHostPath = attrHostPath.parent_path() / attrTypeName;

    auto attrHostParentPath = attrHostPath.parent_path();
    if (!std::filesystem::exists(attrHostParentPath))
    {
        std::filesystem::create_directories(attrHostParentPath);
    }

    if (pos == 0)
    {
        std::ofstream fout(attrHostPath, std::ios::binary);
        if (!fout.is_open())
        {
            return B_ENTRY_NOT_FOUND;
        }

        fout.write((const char*)buffer, size);
    }
    else
    {
        std::ofstream fout(attrHostPath, std::ios::binary | std::ios::app);
        if (!fout.is_open())
        {
            return B_ENTRY_NOT_FOUND;
        }

        fout.seekp(pos, std::ios::beg);
        fout.write((const char*)buffer, size);
    }

    std::ofstream fout(attrTypeHostPath, std::ios::binary);
    fout.write((const char*)&type, sizeof(type));

    return size;
}

haiku_ssize_t PackagefsDevice::RemoveAttr(const std::filesystem::path& path, const std::string& name)
{
    auto relativePath = path.lexically_relative(_root);
    auto attrRelativePath = _GetAttrPathInternal(relativePath, name);

    auto attrHostPath = _hostRoot / attrRelativePath;
    auto attrTypeName = attrHostPath.filename().string();
    attrTypeName[0] = PACKAGEFS_ATTREMU_TYPE;
    auto attrTypeHostPath = attrHostPath.parent_path() / attrTypeName;

    std::error_code ec;
    std::filesystem::remove(attrHostPath, ec);

    if (ec)
    {
        return CppToB(ec);
    }

    std::filesystem::remove(attrTypeHostPath, ec);
    return B_OK;
}

status_t PackagefsDevice::Mount(const std::filesystem::path& path,
    const std::filesystem::path& device, uint32 flags, const std::string& args,
    std::shared_ptr<VfsDevice>& output)
{
    PackageFSMountType mountType = PACKAGE_FS_MOUNT_TYPE_SYSTEM;

    if (!args.empty())
    {
        std::stringstream ss(args);
        std::string arg;

        ss >> arg;

        if (arg != "type")
        {
            return B_BAD_VALUE;
        }

        ss >> arg;

        if (arg == "system")
        {
            mountType = PACKAGE_FS_MOUNT_TYPE_SYSTEM;
        }
        else if (arg == "home")
        {
            mountType = PACKAGE_FS_MOUNT_TYPE_HOME;
        }
        else if (arg == "custom")
        {
            mountType = PACKAGE_FS_MOUNT_TYPE_CUSTOM;
        }
        else
        {
            return B_BAD_VALUE;
        }
    }

    auto& vfsService = System::GetInstance().GetVfsService();
    auto hostPath = path;
    status_t status = vfsService.GetPath(hostPath, false);

    if (status != B_OK)
    {
        return status;
    }

    output = std::make_shared<PackagefsDevice>(path, hostPath, mountType, flags);

    return B_OK;
}