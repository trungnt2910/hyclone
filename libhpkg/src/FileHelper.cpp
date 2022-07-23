#include <fstream>

#include <magic_enum.hpp>

#include <libhpkg/FileHelper.h>
#include <libhpkg/Helpers/StringHelpers.h>
#include <libhpkg/HpkException.h>
#include <libhpkg/Model/FileType.h>

namespace LibHpkg
{
    using Model::FileType;

    static constexpr auto fileTypeValues = magic_enum::enum_values<FileType>();

    FileType FileHelper::GetType(std::ifstream& file) const
    {
        auto result = TryGetType(file);
        return result.has_value() ? result.value() : throw HpkException("unable to establish the file type");
    }

    std::optional<FileType> FileHelper::TryGetType(std::ifstream& file) const
    {
        std::array<char, 4> magic = ReadMagic(file);

        std::string magicString(magic.begin(), magic.end());
        for (const auto& value : fileTypeValues)
        {
            if (Helpers::tolower(std::string(magic_enum::enum_name(value))) == magicString)
            {
                return value;
            }
        }
        return std::nullopt;
    }

    int32_t FileHelper::ReadUnsignedShortToInt(std::ifstream& file) const
    {
        file.read(reinterpret_cast<char*>(buffer8), 2);

        if (file.eof())
        {
            throw HpkException("not enough bytes read for an unsigned short");
        }

        int i0 = buffer8[0] & 0xff;
        int i1 = buffer8[1] & 0xff;

        return i0 << 8 | i1;
    }

    int64_t FileHelper::ReadUnsignedIntToLong(std::ifstream& file) const
    {
        file.read(reinterpret_cast<char*>(buffer8), 4);

        if (file.eof())
        {
            throw HpkException("not enough bytes read for an unsigned int");
        }

        int64_t l0 = buffer8[0] & 0xff;
        int64_t l1 = buffer8[1] & 0xff;
        int64_t l2 = buffer8[2] & 0xff;
        int64_t l3 = buffer8[3] & 0xff;

        return l0 << 24 | l1 << 16 | l2 << 8 | l3;
    }

    mpz_class FileHelper::ReadUnsignedLong(std::ifstream& file) const
    {
        file.read(reinterpret_cast<char*>(buffer8), 8);

        if (file.eof())
        {
            throw HpkException("not enough bytes read for an unsigned long");
        }

        mpz_class big;
        mpz_import(
            // mpz_t target
            big.get_mpz_t(),
            // count of buffer
            sizeof(uint64_t), 
            // order of bytes in buffer
            1, 
            // size of each buffer element
            1, 
            // endian (use big-endian because that's what the server on Java does)
            1,
            // nails (don't know wtf that is)
            0,
            buffer8);
        
        return big;
    }

    int64_t FileHelper::ReadUnsignedLongToLong(std::ifstream& file) const
    {
        file.read(reinterpret_cast<char*>(buffer8), 8);

        if (file.eof())
        {
            throw HpkException("not enough bytes read for an unsigned long");
        }

        uint64_t l0 = buffer8[0] & 0xff;
        uint64_t l1 = buffer8[1] & 0xff;
        uint64_t l2 = buffer8[2] & 0xff;
        uint64_t l3 = buffer8[3] & 0xff;
        uint64_t l4 = buffer8[4] & 0xff;
        uint64_t l5 = buffer8[5] & 0xff;
        uint64_t l6 = buffer8[6] & 0xff;
        uint64_t l7 = buffer8[7] & 0xff;
        
        uint64_t res = l0 << 56 | l1 << 48 | l2 << 40 | l3 << 32 | l4 << 24 | l5 << 16 | l6 << 8 | l7;

        if (res > MAX_BIGINTEGER_FILE)
        {
            throw HpkException("unsigned long is too large");
        }

        return res;
    }

    std::array<char, 4> FileHelper::ReadMagic(std::ifstream& file) const
    {
        std::array<char, 4> magic;
        file.read(magic.data(), magic.size());
        return magic;
    }
}