#ifndef __HPKGVFS_PACKAGE_H__
#define __HPKGVFS_PACKAGE_H__

#include <memory>
#include <string>
#include <vector>

#include <libhpkg/HpkgFileExtractor.h>

#include "Entry.h"

namespace HpkgVfs
{
    class Package
    {
    private:
        std::shared_ptr<LibHpkg::HpkgFileExtractor> _extractor;
        std::shared_ptr<LibHpkg::AttributeContext> _attributesContext;
        std::shared_ptr<LibHpkg::AttributeContext> _tocContext;

        std::string _name;
        std::string _version;
        std::string _architecture;

        std::vector<std::string> _requries;
        std::vector<std::string> _writableFiles;
        std::vector<std::string> _writableDirectories;
    public:
        Package(const std::string& name);

        // Gets the package ID (name-version-architecture)
        std::string GetId() const
        {
            return _name + "-" + _version + "-" + _architecture;
        }

        // Install folder ID (the one in package-links)
        std::string GetInstallFolderId() const
        {
            return _name + "-" + _version;
        }
        
        const std::vector<std::string>& GetRequiredPackages() const
        {
            return _requries;
        }
        const std::vector<std::string>& GetWritableFiles() const
        {
            return _writableFiles;
        }
        const std::vector<std::string>& GetWritableDirectories() const
        {
            return _writableDirectories;
        }
        
        std::shared_ptr<Entry> GetRootEntry(bool dropData = false) const;
    };
}

#endif // __HPKGVFS_PACKAGE_H__