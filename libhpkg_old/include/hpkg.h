#ifndef __HYCLONE_HPKG_H__
#define __HYCLONE_HPKG_H__

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace Hpkg
{
    inline constexpr uint64_t MAX_TOC = 64 * 1024 * 1024;
    inline constexpr uint64_t MAX_ATTRIBUTES = 1 * 1024 * 1024;

    enum AttributeID
    {
        DirectoryEntry,
        FileType,
        FilePermissions,
        FileUser,
        FileGroup,
        FileAtime,
        FileMtime,
        FileCrtime,
        FileAtimeNanos,
        FileMtimeNanos,
        FileCrtimNanos,
        FileAttribute,
        FileAttributeType,
        Data,
        DataSize,
        DataCompression,
        DataChunkSize,
        SymlinkPath,
        PackageName,
        PackageSummary,
        PackageDescription,
        PackageVendor,
        PackagePackager,
        PackageFlags,
        PackageArchitecture,
        PackageVersionMajor,
        PackageVersionMinor,
        PackageVersionMicro,
        PackageVersionRevision,
        PackageCopyright,
        PackageLicense,
        PackageProvides,
        PackageProvidesType,
        PackageRequires,
        PackageSupplements,
        PackageConflicts,
        PackageFreshens,
        PackageReplaces,
        PackageResolvableOperator,
        PackageChecksum,
        PackageVersionPreRelease,
        PackageProvidesCompatible,
        PackageUrl,
        PackageSourceUrl,
        PackageInstallPath,

        EnumCount
    };

    struct PackageHeaderV2
    {
        uint32_t magic;
        uint16_t header_size;
        uint16_t version;
        uint64_t total_size;
        uint16_t minor_version;

        // Heap
        uint16_t heap_compression;
        uint32_t heap_chunk_size;
        uint64_t heap_size_compressed;
        uint64_t heap_size_uncompressed;

        // package attributes section
        uint32_t attributes_length;
        uint32_t attributes_strings_length;
        uint32_t attributes_strings_count;
        uint32_t reserved1;

        // TOC section
        uint64_t toc_length;
        uint64_t toc_strings_length;
        uint64_t toc_strings_count;
    };

    struct PackageFileSection
    {
        uint32_t    uncompressed_length;
        uint8_t     data; // TODO: Data uint8*
        uint64_t    offset;
        uint64_t    current_offset;
        uint64_t    strings_length;
        uint64_t    strings_count;
        uint8_t     strings; // TODO: char**
        std::string name;
    };

    class Package 
    {
    private:
        void parse_header();
        uint64_t heap_chunk_count();
        uint64_t heap_chunk_uncompressed_size(uint64_t index);
        uint64_t heap_chunk_offset(uint64_t index);
        size_t inflate_heap_chunk(uint64_t index);
        size_t inflate_heap();
    public:
        std::optional<std::filesystem::path>    filename;
        std::optional<PackageHeaderV2>          header;

        std::optional<std::string>  name;
        std::optional<std::string>  summary;
        std::optional<std::string>  description;
        std::optional<std::string>  vendor;
        std::optional<std::string>  packager;
        std::optional<int32_t>      basepackage;
        std::optional<std::string>  checksum;
        std::optional<std::string>  installpath;
        uint32_t                    flags;
        std::optional<std::string>  architecture;

        std::vector<std::vector<uint8_t>> heap_data;

        Package() = default;
        static Package load(const std::filesystem::path& hpkg_file);
    };

    namespace Private
    {
        template <typename T>
        concept Read = std::is_base_of<std::istream, T>::value;

        template <typename T, Read R>
        inline T read_struct(R& r)
        {
            T t;
            memset(&t, 0, sizeof(T));
            r.read(reinterpret_cast<char*>(&t), sizeof(t));
            return t;
        }


        template<std::integral T>
        constexpr T byteswap(T value) noexcept
        {
            static_assert(std::has_unique_object_representations_v<T>, 
                        "T may not have padding bits");
            auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
            std::reverse(value_representation.begin(), value_representation.end());
            return std::bit_cast<T>(value_representation);
        }
    }

    class HpkgException : std::runtime_error
    {
    public:
        HpkgException(const std::string& what) : std::runtime_error(what) {}
        HpkgException(const char* what) : std::runtime_error(what) {}
    };

} // namespace Hpkg

std::ostream& operator<<(std::ostream& os, const Hpkg::Package& hpkg);

#endif // __HYCLONE_HPKG_H__