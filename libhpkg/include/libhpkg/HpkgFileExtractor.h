#ifndef __LIBHPKG_HPKGFILEEXTRACTOR_H__
#define __LIBHPKG_HPKGFILEEXTRACTOR_H__

#include <filesystem>
#include <memory>
#include <vector>

#include "HpkgHeader.h"
#include "Heap/HpkHeapReader.h"
#include "Model/Attribute.h"

namespace LibHpkg
{
    class AttributeContext;
    class HpkStringTable;
    class AttributeIterator;

    /// <summary>
    /// <para>This object represents an object that can extract an Hpkg (Haiku Pkg) file.  If you are wanting to
    /// read HPKG files then you should instantiate an instance of this class and then make method calls to it in order to
    /// read values such as the attributes of the file.</para>
    /// </summary>
    class HpkgFileExtractor
    {
    private:
        const std::filesystem::path file;

        const HpkgHeader header;

        const std::shared_ptr<Heap::HpkHeapReader> heapReader;

        const std::shared_ptr<HpkStringTable> tocStringTable;

        const std::shared_ptr<HpkStringTable> packageAttributesStringTable;

    public:
        HpkgFileExtractor(const std::filesystem::path& file);

        void Close()
        {
            if (heapReader)
            {
                heapReader->Close();
            }
        }

        std::shared_ptr<AttributeContext> GetPackageAttributeContext() const;

        std::shared_ptr<AttributeIterator> GetPackageAttributesIterator() const;

        std::vector<std::shared_ptr<Model::Attribute>> GetPackageAttributes() const;

        std::shared_ptr<AttributeContext> GetTocContext() const;

        std::shared_ptr<AttributeIterator> GetTocIterator() const;

        std::vector<std::shared_ptr<Model::Attribute>> GetToc() const;

        HpkgHeader ReadHeader() const;

        const std::filesystem::path& GetFile() const
        {
            return file;
        }
    };
}

#endif // __LIBHPKG_HPKGFILEEXTRACTOR_H__