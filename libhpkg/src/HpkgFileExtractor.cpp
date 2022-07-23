#include <cassert>

#include <magic_enum.hpp>

#include <libhpkg/AttributeIterator.h>
#include <libhpkg/Heap/HpkHeapReader.h>
#include <libhpkg/HpkException.h>
#include <libhpkg/HpkStringTable.h>
#include <libhpkg/HpkgFileExtractor.h>
#include <libhpkg/Model/Attribute.h>
#include <libhpkg/StringTable.h>

namespace LibHpkg
{
    using namespace LibHpkg::Heap;
    using namespace LibHpkg::Model;

    HpkgFileExtractor::HpkgFileExtractor(const std::filesystem::path& file)
    try
        : file(file), heapReader(
            std::make_shared<HpkHeapReader>(
                file,
                header.HeapCompression,
                header.HeaderSize,
                header.HeapChunkSize, // uncompressed size
                header.HeapSizeCompressed, // including the compressed chunk lengths.
                header.HeapSizeUncompressed // excludes the compressed chunk lengths.
            )
        ), tocStringTable(
            std::make_shared<HpkStringTable>(
                heapReader,
                header.HeapSizeUncompressed
                        - (header.PackageAttributesLength + header.TocLength),
                header.TocStringsLength,
                header.TocStringsCount
            )
        ), packageAttributesStringTable(
            std::make_shared<HpkStringTable>(
                heapReader,
                header.HeapSizeUncompressed - header.PackageAttributesLength,
                header.PackageAttributesStringsLength,
                header.PackageAttributesStringsCount
            )
        ), header(ReadHeader())
    {
        // Should not use an assert here but use an exception.
        if (!std::filesystem::exists(file) || std::filesystem::is_directory(file))
        {
            throw std::runtime_error("the file does not exist or is not a file.");
        }

    }
    catch (const std::exception& e)
    {
        Close();
        throw HpkException("unable to setup the hpkg file extractor", e);
    }
    catch (...)
    {
        Close();
        throw HpkException("unable to setup the hpkg file extractor");
    }

    std::shared_ptr<AttributeContext> HpkgFileExtractor::GetPackageAttributeContext() const
    {
        auto context = std::make_shared<AttributeContext>();
        context->HeapReader = heapReader;
        context->StringTable = dynamic_pointer_cast<StringTable>(packageAttributesStringTable);
        return context;
    }

    std::shared_ptr<AttributeIterator> HpkgFileExtractor::GetPackageAttributesIterator() const
    {
        size_t offset = (header.HeapSizeUncompressed - header.PackageAttributesLength)
            + header.PackageAttributesStringsLength;
        return std::make_shared<AttributeIterator>(GetPackageAttributeContext(), offset);
    }

    std::shared_ptr<AttributeContext> HpkgFileExtractor::GetTocContext() const
    {
        auto context = std::make_shared<AttributeContext>();
        context->HeapReader = heapReader;
        context->StringTable = dynamic_pointer_cast<StringTable>(tocStringTable);
        return context;
    }

    std::shared_ptr<AttributeIterator> HpkgFileExtractor::GetTocIterator() const
    {
        size_t tocOffset = (header.HeapSizeUncompressed
                - (header.PackageAttributesLength + header.TocLength));
        size_t tocAttributeOffset = tocOffset + header.TocStringsLength;
        return std::make_shared<AttributeIterator>(GetTocContext(), tocAttributeOffset);
    }

    std::vector<std::shared_ptr<Attribute>> HpkgFileExtractor::GetToc() const
    {
        std::vector<std::shared_ptr<Attribute>> assembly;
        auto attributeIterator = GetTocIterator();
        while (attributeIterator->HasNext())
        {
            assembly.push_back(attributeIterator->Next());
        }
        return assembly;
    }

    HpkgHeader HpkgFileExtractor::ReadHeader() const
    {
        FileHelper fileHelper;
        std::ifstream randomAccessFile(file, std::ios::in | std::ios::binary);

        if (fileHelper.GetType(randomAccessFile) != FileType::HPKG)
        {
            throw HpkException("magic incorrect at the start of the hpkg file");
        }

        HpkgHeader result;

        result.HeaderSize = fileHelper.ReadUnsignedShortToInt(randomAccessFile);
        result.Version = fileHelper.ReadUnsignedShortToInt(randomAccessFile);
        result.TotalSize = fileHelper.ReadUnsignedLongToLong(randomAccessFile);
        result.MinorVersion = fileHelper.ReadUnsignedShortToInt(randomAccessFile);

        result.HeapCompression = magic_enum::enum_cast<HeapCompression>(fileHelper.ReadUnsignedShortToInt(randomAccessFile)).value();
        result.HeapChunkSize = fileHelper.ReadUnsignedIntToLong(randomAccessFile);
        result.HeapSizeCompressed = fileHelper.ReadUnsignedLongToLong(randomAccessFile);
        result.HeapSizeUncompressed = fileHelper.ReadUnsignedLongToLong(randomAccessFile);

        result.PackageAttributesLength = fileHelper.ReadUnsignedIntToLong(randomAccessFile);
        result.PackageAttributesStringsLength = fileHelper.ReadUnsignedIntToLong(randomAccessFile);
        result.PackageAttributesStringsCount = fileHelper.ReadUnsignedIntToLong(randomAccessFile);
        randomAccessFile.seekg(4, std::ios_base::cur); // reserved uint32

        result.TocLength = fileHelper.ReadUnsignedLongToLong(randomAccessFile);
        result.TocStringsLength = fileHelper.ReadUnsignedLongToLong(randomAccessFile);
        result.TocStringsCount = fileHelper.ReadUnsignedLongToLong(randomAccessFile);

        return result;
    }
}