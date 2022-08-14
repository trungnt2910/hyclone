#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <libhpkg/HpkgFileExtractor.h>
#include <libhpkg/Model/PkgArchitecture.h>

#include <hpkgvfs/Package.h>
#include <hpkgvfs/Entry.h>

namespace HpkgVfs
{
    using namespace LibHpkg;
    using namespace LibHpkg::Model;

    Package::Package(const std::string& filePath)
    {
        _extractor = std::make_shared<HpkgFileExtractor>(std::filesystem::path(filePath));
        _attributesContext = _extractor->GetPackageAttributeContext();
        _tocContext = _extractor->GetTocContext();

        auto packageAttributes = _extractor->GetPackageAttributes();

        for (const auto& attr : packageAttributes)
        {
            if (attr->GetAttributeId() == AttributeId::PACKAGE_NAME)
            {
                _name = attr->GetValue<std::string>(_attributesContext);
            }
            else if (attr->GetAttributeId() == AttributeId::PACKAGE_ARCHITECTURE)
            {
                int architecture = attr->GetValue<mpz_class>(_attributesContext).get_si();
                switch (architecture)
                {
                    case (int)PkgArchitecture::X86_64:
                        _architecture = "x86_64";
                        break;
                    case (int)PkgArchitecture::X86:
                        _architecture = "x86";
                        break;
                    case (int)PkgArchitecture::X86_GCC2:
                        _architecture = "x86_gcc2";
                        break;
                    case (int)PkgArchitecture::ARM:
                        _architecture = "arm";
                        break;
                    case (int)PkgArchitecture::ARM64:
                        _architecture = "arm64";
                        break;
                    case (int)PkgArchitecture::PPC:
                        _architecture = "ppc";
                        break;
                    case (int)PkgArchitecture::RISCV64:
                        _architecture = "riscv64";
                        break;
                    case (int)PkgArchitecture::SOURCE:
                        _architecture = "source";
                        break;
                    case (int)PkgArchitecture::ANY:
                        _architecture = "any";
                        break;
                    default:
                        break;
                }
            }
            else if (attr->GetAttributeId() == AttributeId::PACKAGE_VERSION_MAJOR)
            {
                std::string major = attr->GetValue<std::string>(_attributesContext);
                std::string minor, micro, preRelease;
                std::optional<int> revision;

                auto childAttributes = attr->GetChildAttributes();
                for (const auto & childAttribute: childAttributes)
                {
                    if (childAttribute->GetAttributeId() == LibHpkg::Model::AttributeId::PACKAGE_VERSION_MINOR)
                    {
                        minor = childAttribute->GetValue<std::string>(_attributesContext);
                    }
                    else if (childAttribute->GetAttributeId() == LibHpkg::Model::AttributeId::PACKAGE_VERSION_MICRO)
                    {
                        micro = childAttribute->GetValue<std::string>(_attributesContext);
                    }
                    else if (childAttribute->GetAttributeId() == LibHpkg::Model::AttributeId::PACKAGE_VERSION_PRE_RELEASE)
                    {
                        preRelease = childAttribute->GetValue<std::string>(_attributesContext);
                    }
                    else if (childAttribute->GetAttributeId() == LibHpkg::Model::AttributeId::PACKAGE_VERSION_REVISION)
                    {
                        revision = childAttribute->GetValue<mpz_class>(_attributesContext).get_si();
                    }
                }

                _version += major;
                if (!minor.empty())
                {
                    _version += "." + minor;
                }
                if (!micro.empty())
                {
                    _version += "." + micro;
                }
                if (!preRelease.empty())
                {
                    _version += "~" + preRelease;
                }
                if (revision.has_value())
                {
                    _version += "-" + std::to_string(revision.value());
                }
            }
            else if (attr->GetAttributeId() == AttributeId::PACKAGE_REQUIRES)
            {
                _requries.push_back(attr->GetValue<std::string>(_attributesContext));
            }
            else if (attr->GetAttributeId() == AttributeId::PACKAGE_GLOBAL_WRITABLE_FILE)
            {
                bool isDirectory = false;
                auto childAttr = attr->TryGetChildAttribute(AttributeId::PACKAGE_IS_WRITABLE_DIRECTORY);
                if (childAttr != nullptr)
                {
                    isDirectory = childAttr->GetValue<mpz_class>(_attributesContext).get_si() == 1;
                }
                (isDirectory ? _writableDirectories : _writableFiles).push_back(attr->GetValue<std::string>(_attributesContext));
            }
        }
    }

    std::shared_ptr<Entry> Package::GetRootEntry(bool dropData) const
    {
        std::string name = GetId();

        const std::filesystem::perms defaultPerms =
            std::filesystem::perms::owner_read |
            std::filesystem::perms::owner_exec |
            std::filesystem::perms::group_read |
            std::filesystem::perms::group_exec |
            std::filesystem::perms::others_exec;

        std::shared_ptr<Entry> entry = std::make_shared<Entry>(
            "system",
            std::filesystem::file_type::directory,
            defaultPerms,
            "",
            "",
            std::chrono::file_clock::now(),
            std::chrono::file_clock::now(),
            std::chrono::file_clock::now(),
            name);

        std::shared_ptr<Entry> cache = std::make_shared<Entry>(
            ".hpkgvfsPackages",
            std::filesystem::file_type::directory,
            defaultPerms,
            "",
            "",
            std::chrono::file_clock::now(),
            std::chrono::file_clock::now(),
            std::chrono::file_clock::now(),
            name);

        std::shared_ptr<Entry> package_links = std::make_shared<Entry>(
            "package-links",
            std::filesystem::file_type::directory,
            defaultPerms,
            "",
            "",
            std::chrono::file_clock::now(),
            std::chrono::file_clock::now(),
            std::chrono::file_clock::now(),
            name);

        std::shared_ptr<Entry> package_links_current = std::make_shared<Entry>(
            GetInstallFolderId(),
            std::filesystem::file_type::directory,
            defaultPerms,
            "",
            "",
            std::chrono::file_clock::now(),
            std::chrono::file_clock::now(),
            std::chrono::file_clock::now(),
            name);

        std::shared_ptr<Entry> package_links_self = std::make_shared<Entry>(
            ".self",
            std::filesystem::file_type::symlink,
            defaultPerms,
            "",
            "",
            std::chrono::file_clock::now(),
            std::chrono::file_clock::now(),
            std::chrono::file_clock::now(),
            name);

        std::shared_ptr<Entry> package = std::make_shared<Entry>(
            name + ".hpkg",
            std::filesystem::file_type::regular,
            std::filesystem::perms::owner_read |
            std::filesystem::perms::group_read |
            std::filesystem::perms::others_read,
            "",
            "",
            std::chrono::file_clock::now(),
            std::chrono::file_clock::now(),
            std::chrono::file_clock::now(),
            name);

        {
            if (!dropData)
            {
                std::ifstream packageFile(_extractor->GetFile(), std::ios_base::binary);

                std::vector<uint8_t> buffer;
                std::transform(std::istreambuf_iterator<char>(packageFile), std::istreambuf_iterator<char>(),
                    std::back_inserter(buffer), [](char c) { return static_cast<uint8_t>(c); });

                package->SetData(std::move(buffer));
            }
            else
            {
                // This updates the internal state in the entry, preventing
                // an operation that writes an empty file to disk.
                package->Drop(true);
            }
        }

        package_links_self->SetTarget("../..");

        package_links_current->AddChild(package_links_self);

        for (const auto& r : _requries)
        {
            std::string newName = r;
            for (auto& ch: newName)
            {
                if (ch == ':')
                {
                    ch = '~';
                }
            }

            std::shared_ptr<Entry> package_links_required = std::make_shared<Entry>(
                newName,
                std::filesystem::file_type::symlink,
                defaultPerms,
                "",
                "",
                std::chrono::file_clock::now(),
                std::chrono::file_clock::now(),
                std::chrono::file_clock::now(),
                name);

            package_links_required->SetTarget("../..");

            package_links_current->AddChild(package_links_required);
        }

        package_links->AddChild(package_links_current);

        cache->AddChild(package);

        entry->AddChild(cache);
        entry->AddChild(package_links);

        for (const auto& child : _extractor->GetToc())
        {
            auto temp = Entry::FromAttribute(name, child, _tocContext, dropData);
            entry->AddChild(temp);
        }

        for (const auto& writableFile: _writableFiles)
        {
            auto file = entry->GetChild(writableFile);
            if (file)
            {
                file->AddPermissions(
                    std::filesystem::perms::owner_write |
                    std::filesystem::perms::group_write |
                    std::filesystem::perms::others_write);
            }
        }

        for (const auto& writableDirectory: _writableDirectories)
        {
            auto dir = entry->GetChild(writableDirectory);
            if (dir)
            {
                dir->AddPermissions(
                    std::filesystem::perms::owner_write |
                    std::filesystem::perms::group_write |
                    std::filesystem::perms::others_write);
            }
        }

        // For convenience, the file itself is added to the archive as well
        // and can be extracted later, but it will be ignored by packagefs.
        // https://www.haiku-os.org/docs/develop/packages/BuildingPackages.html
        if (entry->GetChild(std::string(".PackageInfo")) != nullptr)
        {
            entry->RemoveChild(".PackageInfo");
        }

        return entry;
    }
}