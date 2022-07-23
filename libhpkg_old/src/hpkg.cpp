#include "hpkg.h"

#include <fstream>
#include <ranges>

#include <zlib.h>
#include <zfstream.h>

std::ostream& operator<<(std::ostream& os, const Hpkg::Package& hpkg)
{
    return os   << "package. "
                << "Name " << hpkg.name.value_or("") << ", "
                << "Vendor " << hpkg.vendor.value_or("") << ", "
                << "Summary " << hpkg.summary.value_or("") << ", "
                << "Arch " << hpkg.architecture.value_or("");
}

namespace Hpkg
{
    using namespace Private;

	/// Parse the header of a hpkg and populate Package
    void Package::parse_header()
    {
        auto filename = 
            this->filename.has_value() ? this->filename.value() : throw HpkgException("Package filename missing!");
        
        std::ifstream f;
        f.open(filename, std::ios_base::binary);
        f.seekg(0, std::ios_base::beg);

        auto header = read_struct<PackageHeaderV2>(f);
        char magic[] = { 'h', 'p', 'k', 'g' };
        if (header.magic != std::bit_cast<uint32_t>(magic))
        {
            throw HpkgException("Unknown magic: " + std::to_string(header.magic));
        }

        if constexpr (std::endian::native == std::endian::little)
        {
            // Endian Adjustments (are there better ways to do this?)
            header.header_size = byteswap(header.header_size);
            header.version = byteswap(header.version);
            header.total_size = byteswap(header.total_size);
            header.minor_version = byteswap(header.minor_version);
            header.heap_compression = byteswap(header.heap_compression);
            header.heap_chunk_size = byteswap(header.heap_chunk_size);
            header.heap_size_compressed = byteswap(header.heap_size_compressed);
            header.heap_size_uncompressed = byteswap(header.heap_size_uncompressed);
            header.attributes_length = byteswap(header.attributes_length);
            header.attributes_strings_length = byteswap(header.attributes_strings_length);
            header.attributes_strings_count = byteswap(header.attributes_strings_count);
            header.reserved1 = byteswap(header.reserved1);
            header.toc_length = byteswap(header.toc_length);
            header.toc_strings_length = byteswap(header.toc_strings_length);
            header.toc_strings_count = byteswap(header.toc_strings_count);
        }

        if (header.version != 2)
        {
            throw new HpkgException("Unsupported hpkg version: " + std::to_string(header.version));
        }

		// If the minor version of a package/repository file is greater than the
		// current one unknown attributes are ignored without error.

		// TOC and attributes are at the end of the heap section
		if ((uint64_t)header.header_size + header.heap_size_compressed != header.total_size) 
        {
			throw new HpkgException("Invalid hpkg header lengths");
		}

        this->header = header;
    }

    uint64_t Package::heap_chunk_count()
    {
        auto& header = this->header.value();
        auto count = header.heap_size_uncompressed / (uint64_t)header.heap_chunk_size;
        if (header.heap_size_uncompressed % (uint64_t)header.heap_chunk_size != 0)
        {
            ++count;
        }
        return count;
    }

    uint64_t Package::heap_chunk_uncompressed_size(uint64_t index)
    {
        auto header = this->header.value();
        auto chunk_size = (uint64_t)header.heap_chunk_size;
        auto chunk_count = this->heap_chunk_count();
        if (index < chunk_count)
        {
            return chunk_size;
        }
        return header.heap_size_uncompressed - (chunk_size * (chunk_count - 1));
    }

    uint64_t Package::heap_chunk_offset(uint64_t index)
    {
        auto& header = this->header.value();
        return (uint64_t)header.header_size + (index * (uint64_t)header.heap_chunk_size);
    }

    size_t Package::inflate_heap_chunk(uint64_t index)
    {
        auto pos = this->heap_chunk_offset(index);
        auto uncompressed_size = (size_t)this->heap_chunk_uncompressed_size(index);
        auto& header = this->header.value();
        auto& filename = this->filename.value();

        std::cerr   << "Chunk " << index << "; " 
                    << pos << " - " << pos + (uint64_t)header.heap_chunk_size
                    << " inflate to " << uncompressed_size << std::endl;

        std::vector<uint8_t> buffer(uncompressed_size);

        switch (header.heap_compression)
        {
            case 0:
            {
                std::ifstream f(filename, std::ios_base::binary);
                f.seekg(pos, std::ios_base::beg);
                f.read((char *)buffer.data(), uncompressed_size);
                break;
            }
            case 1:
            {
                gzifstream zf(filename.c_str(), std::ios_base::binary);
                zf.seekg(pos, std::ios_base::beg);
                zf.read((char *)buffer.data(), uncompressed_size);
                break;
            }
        }

        this->heap_data.push_back(std::move(buffer));

        return uncompressed_size;
    }

	/// Inflate the heap section of a hpkg for later processing
	/// XXX: This will likely need reworked... just trying to figure out what's going on
    size_t Package::inflate_heap()
    {
        auto chunks = this->heap_chunk_count();
        auto& header = this->header.value();

        std::cerr << "Heap chunk size: " << header.heap_chunk_size << std::endl;
        std::cerr << "Heap chunk count: " << chunks << std::endl;
        std::cerr << "Heap size compressed: " << header.heap_size_compressed << std::endl;
        std::cerr << "Heap size uncompressed: " << header.heap_size_uncompressed << std::endl;
        std::cerr << "Heap compression: " << header.heap_compression << std::endl;

        for (const auto& chunk_index : std::ranges::iota_view((decltype(chunks))0, chunks))
        {
            this->inflate_heap_chunk(chunk_index);
        }

        return 0;
    }

    /// Section start calculated as endOffset - section length
    ///   Attributes Section = uncompressed heap size - attributes section length
    ///   TOC Section = Attributes Section offset - toc section length

    /// Open an hpkg file produce a populated Package representation
    Package Package::load(const std::filesystem::path& hpkg_file)
    {
        std::ifstream f(hpkg_file, std::ios_base::binary);
        f.seekg(0, std::ios_base::beg);

        Package hpkg;
        hpkg.filename = hpkg_file;
        hpkg.parse_header();
        hpkg.inflate_heap();

        return hpkg;
    }
}