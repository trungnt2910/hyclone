#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unordered_map>

#include <hpkgvfs/Entry.h>
#include <hpkgvfs/Package.h>

#include "haiku_fs_info.h"
#include "server_filesystem.h"
#include "server_prefix.h"

bool server_setup_rootfs(const std::filesystem::path& prefix, haiku_fs_info& info)
{
    // same as in Haiku.
    info.flags = 0;
    info.block_size = 0;
    info.io_size = 0;
    info.total_blocks = 0;
    info.free_blocks = 0;
    info.total_nodes = 0;
    info.free_nodes = 0;
    info.device_name[0] = '\0';
    info.volume_name[0] = '\0';
    strncpy(info.fsh_name, "rootfs", sizeof(info.fsh_name));

    struct stat statbuf;
    if (stat(prefix.c_str(), &statbuf) != 0)
    {
        return false;
    }
    info.dev = (haiku_dev_t)statbuf.st_dev;
    info.root = (haiku_ino_t)statbuf.st_ino;

    auto binPath = prefix / "bin";
    auto libPath = prefix / "lib";
    auto etcPath = prefix / "etc";
    auto packagesPath = prefix / "packages";
    auto systemRootPath = prefix / "SystemRoot";

    try
    {
        // Generate some essential symlinks.
        std::filesystem::remove(binPath);
        std::filesystem::create_directory_symlink("boot/system/bin", binPath);
        std::filesystem::remove(libPath);
        std::filesystem::create_directory_symlink("boot/system/lib", libPath);
        std::filesystem::remove(etcPath);
        std::filesystem::create_directory_symlink("boot/system/etc", etcPath);
        std::filesystem::remove(packagesPath);
        std::filesystem::create_directory_symlink("boot/system/package-links", packagesPath);
        std::filesystem::remove(systemRootPath);
        std::filesystem::create_directory(systemRootPath);;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
}

bool server_setup_devfs(haiku_fs_info& info)
{
    // same as in Haiku.
    info.flags = 0;
    info.block_size = 0;
    info.io_size = 0;
    info.total_blocks = 0;
    info.free_blocks = 0;
    info.total_nodes = 0;
    info.free_nodes = 0;
    info.device_name[0] = '\0';
    info.volume_name[0] = '\0';
    strncpy(info.fsh_name, "devfs", sizeof(info.fsh_name));

    struct stat statbuf;
    if (stat("/dev", &statbuf) != 0)
    {
        return false;
    }
    info.dev = (haiku_dev_t)statbuf.st_dev;
    info.root = (haiku_ino_t)statbuf.st_ino;

    return true;
}

bool server_setup_systemfs(haiku_fs_info& info)
{
    info.flags = B_FS_IS_PERSISTENT | B_FS_IS_SHARED | B_FS_HAS_ATTR;

    struct statfs statfsbuf;
    if (statfs("/", &statfsbuf) != 0)
    {
        return false;
    }

    info.block_size = statfsbuf.f_frsize;
    info.io_size = statfsbuf.f_bsize;
    info.total_blocks = statfsbuf.f_blocks;
    info.free_blocks = statfsbuf.f_bfree;
    info.total_nodes = statfsbuf.f_files;
    info.free_nodes = statfsbuf.f_ffree;
    info.device_name[0] = '\0';
    strncpy(info.volume_name, "SystemRoot", sizeof(info.volume_name));
    strncpy(info.fsh_name, "systemfs", sizeof(info.fsh_name));

    struct stat statbuf;
    if (stat("/", &statbuf) != 0)
    {
        return false;
    }
    info.dev = (haiku_dev_t)statbuf.st_dev;
    info.root = (haiku_ino_t)statbuf.st_ino;

    return true;
}

bool server_setup_packagefs(const std::filesystem::path& root, haiku_fs_info& info)
{
    using namespace HpkgVfs;

    std::filesystem::path packagesPath = root / "boot" / "system" / "packages";
    std::filesystem::path installedPackagesPath = root / "boot" / "system" / ".hpkgvfsPackages";

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
            boot->WriteToDisk(root);
        }
        else if (it->second > kvp.second)
        {
            std::cerr << "Updating package: " << kvp.first << " (later timestamp detected)" << std::endl;
            boot->RemovePackage(kvp.first);
            Package package((packagesPath / (it->first + ".hpkg")).string());
            std::shared_ptr<Entry> entry = package.GetRootEntry();
            system->Merge(entry);
            boot->WriteToDisk(root);
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
        boot->WriteToDisk(root);
        boot->Drop();
    }

    std::cerr << "Done extracting packagefs." << std::endl;

    std::filesystem::path systemPath = root / "boot" / "system";

    // To be set by the system.
    info.dev = 0;
    info.flags = B_FS_IS_PERSISTENT | B_FS_IS_SHARED | B_FS_HAS_ATTR;

    struct statfs statfsbuf;
    if (statfs(systemPath.c_str(), &statfsbuf) != 0)
    {
        return false;
    }

    info.block_size = statfsbuf.f_frsize;
    info.io_size = statfsbuf.f_bsize;
    info.total_blocks = statfsbuf.f_blocks;
    info.free_blocks = statfsbuf.f_bfree;
    info.total_nodes = statfsbuf.f_files;
    info.free_nodes = statfsbuf.f_ffree;
    info.device_name[0] = '\0';

    strncpy(info.volume_name, "system", sizeof(info.volume_name));
    strncpy(info.fsh_name, "packagefs", sizeof(info.fsh_name));

    struct stat statbuf;
    if (stat(root.c_str(), &statbuf) != 0)
    {
        return false;
    }
    info.dev = (haiku_dev_t)statbuf.st_dev;
    info.root = (haiku_ino_t)statbuf.st_ino;

    return true;
}

bool server_setup_settings()
{
    std::filesystem::path systemPath = std::filesystem::path(gHaikuPrefix) / "boot" / "system";
    std::filesystem::path settingsPath = systemPath / "settings";
    std::filesystem::path networkSettingsPath = settingsPath / "network";

    std::filesystem::create_directories(networkSettingsPath);

    std::filesystem::copy_file("/etc/hostname", networkSettingsPath / "hostname",
        std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file("/etc/hosts", networkSettingsPath / "hosts",
        std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file("/etc/resolv.conf", networkSettingsPath / "resolv.conf",
        std::filesystem::copy_options::overwrite_existing);

    std::filesystem::path etcPath = systemPath / "etc";
    auto oldSystemPerms = std::filesystem::status(systemPath).permissions();
    std::filesystem::permissions(systemPath, oldSystemPerms | std::filesystem::perms::owner_write);
    std::filesystem::create_directories(etcPath);
    std::filesystem::permissions(systemPath, oldSystemPerms);

    std::filesystem::copy_file("/etc/passwd", etcPath / "passwd",
        std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file("/etc/group", etcPath / "group",
        std::filesystem::copy_options::overwrite_existing);
    std::fstream shadow((etcPath / "shadow").c_str(), std::ios::out | std::ios::app);

    return true;
}
