#include <cassert>
#include <fstream>
#include <iostream>

#include <hpkgvfs/Entry.h>
#include <hpkgvfs/Package.h>

#include "fs/packagefs.h"
#include "server_filesystem.h"
#include "server_native.h"

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
            boot->WriteToDisk(root.parent_path());
        }
        else if (it->second > kvp.second)
        {
            std::cerr << "Updating package: " << kvp.first << " (later timestamp detected)" << std::endl;
            boot->RemovePackage(kvp.first);
            Package package((packagesPath / (it->first + ".hpkg")).string());
            std::shared_ptr<Entry> entry = package.GetRootEntry();
            system->Merge(entry);
            boot->WriteToDisk(root.parent_path());
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
        boot->WriteToDisk(root.parent_path());
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

    server_fill_fs_info(root, &_info);
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