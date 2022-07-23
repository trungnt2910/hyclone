#include <libhpkg/Model/PkgVersion.h>

namespace LibHpkg::Model
{
    void PkgVersion::AppendDotValue(std::string& str, const std::string& value) const
    {
        if (!str.empty())
        {
            str += ".";
        }

        str += value;
    }

    std::string PkgVersion::ToString() const
    {
        std::string result;

        AppendDotValue(result, major);
        AppendDotValue(result, minor);
        AppendDotValue(result, micro);

        if (revision.has_value())
        {
            AppendDotValue(result, std::to_string(revision.value()));
        }

        return result;
    }
}