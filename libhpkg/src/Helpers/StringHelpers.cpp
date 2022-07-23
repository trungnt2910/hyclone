#include <algorithm>
#include <cctype>

#include <libhpkg/Helpers/StringHelpers.h>

namespace LibHpkg::Helpers
{
    std::string trim(const std::string& input)
    {
        auto beg = input.begin();
        auto end = input.end();

        while (beg != end && std::isspace(*beg))
        {
            ++beg;
        }

        while (end >= beg && std::isspace(*(end - 1)))
        {
            --end;
        }

        return std::string(beg, end);
    }

    std::string tolower(const std::string& input)
    {
        std::string result;
        result.reserve(input.size());

        std::transform(input.begin(), input.end(), std::back_inserter(result), [](char c) { return std::tolower(c); });
     
        return result;
    }
}