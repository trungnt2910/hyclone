#ifndef __LIBHPKG_FILEHELPER_H__
#define __LIBHPKG_FILEHELPER_H__

#include <array>
#include <cstdint>
#include <iosfwd>
#include <limits>
#include <optional>

#include <gmpxx.h>

#include "Model/FileType.h"

namespace LibHpkg
{
    class FileHelper
    {      
    private:
        const uint64_t MAX_BIGINTEGER_FILE = std::numeric_limits<std::int64_t>::max();
        mutable uint8_t buffer8[8];

    public:
        Model::FileType GetType(std::ifstream& file) const;
        std::optional<Model::FileType> TryGetType(std::ifstream& file) const;
        int32_t ReadUnsignedShortToInt(std::ifstream& file) const;
        int64_t ReadUnsignedIntToLong(std::ifstream& file) const;
        mpz_class ReadUnsignedLong(std::ifstream& file) const;
        int64_t ReadUnsignedLongToLong(std::ifstream& file) const;
        std::array<char, 4> ReadMagic(std::ifstream& file) const;
    };
}

#endif // __LIBHPKG_FILEHELPER_H__