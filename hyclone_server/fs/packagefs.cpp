#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

#include <hpkgvfs/Entry.h>
#include <hpkgvfs/Package.h>

#include "fs/packagefs.h"
#include "server_filesystem.h"
#include "server_native.h"
#include "system.h"

PackagefsDevice::PackagefsDevice(const std::filesystem::path& root,
    const std::filesystem::path& hostRoot)
    : HostfsDevice(root, hostRoot)
{
    using namespace HpkgVfs;

    std::filesystem::path packagesPath = hostRoot / "system" / "packages";
    std::filesystem::path installedPackagesPath = hostRoot / _relativeInstalledPackagesPath;

    std::shared_ptr<Entry> system;
    std::shared_ptr<Entry> boot = Entry::CreateHaikuBootEntry(&system);

    std::unordered_map<std::string, std::filesystem::file_time_type> installedPackages;
    std::unordered_map<std::string, std::filesystem::file_time_type> enabledPackages;

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
            installedPackages.emplace(file.path().stem(), file.last_write_time());
            std::cerr << "Preinstalled package: " << file.path().stem() << std::endl;
            std::shared_ptr<Entry> entry = package.GetRootEntry(/*dropData*/ true);
            system->Merge(entry);
        }
    }

    std::vector<std::pair<std::filesystem::path, std::string>> toRename;

    for (const auto& file : std::filesystem::directory_iterator(packagesPath))
    {
        if (file.path().extension() != ".hpkg")
        {
            continue;
        }

        // file may be a symlink which we have to resolve.
        if (!std::filesystem::is_regular_file(std::filesystem::status(file.path())))
        {
            continue;
        }

        std::string name;
        std::string fileName;

        {
            Package package(file.path().string());
            name = package.GetId();
            fileName = name + ".hpkg";
        }

        if (file.path().filename() != fileName)
        {
            toRename.emplace_back(file.path(), fileName);
        }

        // Package names might not match the ID.
        // We must read the data from the package.
        enabledPackages.emplace(name, file.last_write_time());
    }

    for (const auto& pair : toRename)
    {
        const std::filesystem::path& oldName = pair.first;
        auto newName = oldName.parent_path() / pair.second;
        std::cerr << "Renaming " << oldName << " to " << newName << std::endl;
        std::filesystem::rename(oldName, newName);
    }

    for (const auto& kvp: installedPackages)
    {
        auto it = enabledPackages.find(kvp.first);
        if (it == enabledPackages.end())
        {
            std::cerr << "Uninstalling package: " << kvp.first << std::endl;
            boot->RemovePackage(kvp.first);
            boot->WriteToDisk(hostRoot.parent_path());
        }
        else if (it->second > kvp.second)
        {
            std::cerr << "Updating package: " << kvp.first << " (later timestamp detected)" << std::endl;
            boot->RemovePackage(kvp.first);
            Package package((packagesPath / (it->first + ".hpkg")).string());
            std::shared_ptr<Entry> entry = package.GetRootEntry();
            system->Merge(entry);
            boot->WriteToDisk(hostRoot.parent_path());
            boot->Drop();
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
        Package package((packagesPath / (pkg.first + ".hpkg")).string());
        std::shared_ptr<Entry> entry = package.GetRootEntry();
        system->Merge(entry);
        boot->WriteToDisk(hostRoot.parent_path());
        boot->Drop();
    }

    std::cerr << "Done extracting packagefs." << std::endl;

    _info.flags = B_FS_IS_PERSISTENT | B_FS_IS_SHARED | B_FS_HAS_ATTR;
    _info.block_size = 0;
    _info.io_size = 0;
    _info.total_blocks = 0;
    _info.free_blocks = 0;
    _info.total_nodes = 0;
    _info.free_nodes = 0;
    strncpy(_info.volume_name, "system", sizeof(_info.volume_name));
    strncpy(_info.fsh_name, "packagefs", sizeof(_info.fsh_name));

    server_fill_fs_info(hostRoot, &_info);
}

bool PackagefsDevice::_IsBlacklisted(const std::filesystem::path& hostPath) const
{
    if (hostPath.lexically_relative(_hostRoot) == _relativeInstalledPackagesPath)
    {
        return true;
    }
    return false;
}

bool PackagefsDevice::_IsBlacklisted(const std::filesystem::directory_entry& entry) const
{
    return _IsBlacklisted(entry.path());
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
            volumeInfo->rootDeviceID = _info.dev;
            volumeInfo->rootDirectoryID = _info.root;
            volumeInfo->packagesDirectoryCount = 1;

            auto& vfsService = System::GetInstance().GetVfsService();

            haiku_stat st;
            status_t status = vfsService.ReadStat(_root / "system" / "packages", st, false);
            if (status != B_OK)
            {
                return status;
            }

            volumeInfo->packagesDirectoryInfos[0].deviceID = st.st_dev;
            volumeInfo->packagesDirectoryInfos[0].nodeID = st.st_ino;

            vfsService.RegisterEntryRef(EntryRef(st.st_dev, st.st_ino), _root / "system" / "packages");

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
            status_t status = vfsService.ReadStat(_root / "system" / "packages", dirst, false);
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

                    auto installedPackagePath = _root / "system" / "packages" / package.path().filename();

                    haiku_stat st;
                    status = vfsService.ReadStat(installedPackagePath, st, false);

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

            std::filesystem::path packagesPath = _hostRoot / "system" / "packages";
            std::filesystem::path installedPackagesPath = _hostRoot / _relativeInstalledPackagesPath;

            std::shared_ptr<Entry> system;
            std::shared_ptr<Entry> boot = Entry::CreateHaikuBootEntry(&system);

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
                    Package package((packagesPath / (name.string() + ".hpkg")).string());
                    std::shared_ptr<Entry> entry = package.GetRootEntry();
                    system->Merge(entry);
                    boot->WriteToDisk(_hostRoot.parent_path());
                    boot->Drop();
                }
                else
                {
                    if (item.type == PACKAGE_FS_DEACTIVATE_PACKAGE)
                    {
                        std::cerr << "Uninstalling package: " << *it << std::endl;
                        boot->RemovePackage(*it);
                        boot->WriteToDisk(_hostRoot.parent_path());
                    }
                    else if (item.type == PACKAGE_FS_REACTIVATE_PACKAGE)
                    {
                        std::cerr << "Reinstalling package: " << *it << std::endl;
                        boot->RemovePackage(*it);
                        Package package((packagesPath / (*it + ".hpkg")).string());
                        std::shared_ptr<Entry> entry = package.GetRootEntry();
                        system->Merge(entry);
                        boot->WriteToDisk(_hostRoot.parent_path());
                        boot->Drop();
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

            std::cerr << "Done updating packagefs." << std::endl;
            return B_OK;
        }
        default:
        {
            return HostfsDevice::Ioctl(path, cmd, addr, buffer, size);
        }
    }
}