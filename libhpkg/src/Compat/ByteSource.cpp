#include <memory>

#include <libhpkg/Compat/ByteArrayByteSource.h>
#include <libhpkg/Compat/ByteSource.h>

namespace LibHpkg::Compat
{
    std::shared_ptr<ByteSource> ByteSource::Wrap(const std::vector<uint8_t>& b)
    {
        return std::make_shared<ByteArrayByteSource>(b);
    }

    std::vector<uint8_t> ByteSource::Read() const
    {
        auto stream = OpenStream();
        std::vector<uint8_t> result;
        result.reserve(SizeIfKnown().value_or(0));

        while (stream->good())
        {
            auto next = stream->get();
            if (next == std::istream::traits_type::eof())
            {
                break;
            }
            result.push_back(next);
        }

        return result;
    }
}