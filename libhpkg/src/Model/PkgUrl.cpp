#include <magic_enum.hpp>

#include <libhpkg/Model/PkgUrl.h>

namespace LibHpkg::Model
{
    const std::regex PkgUrl::PATTERN_NAMEANDURL = std::regex("^([^<>]*)(\\s+)?<([^<> ]+)>$");

    PkgUrl::PkgUrl(const std::string& input, PkgUrlType urlType)
        : urlType(urlType)
    {
        assert(!input.empty());

        auto input_trimmed = Helpers::trim(input);

        if (input_trimmed.empty())
        {
            throw std::invalid_argument("the input must be supplied and should not be an empty string when trimmed.");
        }

        std::smatch matcher;
        std::regex_match(input_trimmed, matcher, PATTERN_NAMEANDURL);

        if (matcher.size())
        {
            url = matcher[3];
            name = Helpers::trim(matcher[1]);
        }
        else
        {
            url = input_trimmed;
            name = "";
        }
    }

    std::string PkgUrl::ToString() const
    {
        return std::string(magic_enum::enum_name(urlType)) + "; " + url;
    }
}