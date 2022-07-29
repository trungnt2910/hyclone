#include <filesystem>
#include <iostream>
#include <memory>
#include <unordered_map>

#include <hpkgvfs/Entry.h>
#include <hpkgvfs/Package.h>

using namespace HpkgVfs;

void PrintFS(const std::shared_ptr<Entry>& boot, int generation = 0)
{
    std::string indent(generation * 2, ' ');
    std::cout << indent << boot->ToString() << std::endl;
    for (const auto& child : boot->GetChildren())
    {
        PrintFS(child, generation + 1);
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <Path to PackageFS root> [--write]" << std::endl;
        return 1;
    }

    bool write = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--write")
        {
            write = true;
        }
    }

    std::filesystem::path root = argv[1];
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
            std::cout << "Preinstalled package: " << file.path().stem() << std::endl;
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
        std::cout << "Renaming " << pair.first << " to " << pair.second << std::endl;
        std::filesystem::rename(pair.first, pair.second);
    }

    for (const auto& kvp: installedPackages)
    {
        auto it = enabledPackages.find(kvp.first);
        if (it == enabledPackages.end())
        {
            std::cout << "Uninstalling package: " << kvp.first << std::endl;
            boot->RemovePackage(kvp.first);
            if (write)
            {
                boot->WriteToDisk(root);
            }
        }
        else if (it->second > kvp.second)
        {
            std::cout << "Updating package: " << kvp.first << " (later timestamp detected)" << std::endl;
            boot->RemovePackage(kvp.first);
            Package package((packagesPath / (it->first + ".hpkg")).string());
            std::shared_ptr<Entry> entry = package.GetRootEntry();
            system->Merge(entry);
            if (write)
            {
                boot->WriteToDisk(root);
            }
            boot->Drop(!write);
        }
        else
        {
            std::cout << "Keeping installed package: " << kvp.first << std::endl;
        }

        if (it != enabledPackages.end())
        {
            enabledPackages.erase(it);
        }
    }

    for (const auto& pkg: enabledPackages)
    {
        std::cout << "Installing package: " << pkg.first << std::endl;
        Package package((packagesPath / (pkg.first + ".hpkg")).string());
        std::shared_ptr<Entry> entry = package.GetRootEntry();
        system->Merge(entry);
        if (write)
        {
            boot->WriteToDisk(root);
        }
        boot->Drop(!write);
    }

    std::cout << "Final file system: " << std::endl;
    PrintFS(boot);

    return 0;
}