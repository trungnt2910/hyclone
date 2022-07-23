#ifndef __LIBHPKG_COMPAT_BYTEARRAYBYTESOURCE_H__
#define __LIBHPKG_COMPAT_BYTEARRAYBYTESOURCE_H__

#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "ByteSource.h"

namespace LibHpkg::Compat
{
    class ByteArrayByteSource: public ByteSource
    {
    private:
        const std::vector<uint8_t> _value;
    public:
        ByteArrayByteSource(const std::vector<uint8_t>& b)
            : _value(b)
        {
        }

        virtual std::shared_ptr<std::istream> OpenStream() const override
        {
            return std::make_shared<std::istringstream>(
                std::string((const char*)_value.data(), _value.size()));
        }

        virtual std::optional<size_t> SizeIfKnown() const override
        {
            return _value.size();
        }

        virtual size_t Size() const override
        {
            return _value.size();
        }

        virtual std::vector<uint8_t> Read() const override
        {
            return _value;
        }
    };
}

#endif // __LIBHPKG_COMPAT_BYTEARRAYBYTESOURCE_H__