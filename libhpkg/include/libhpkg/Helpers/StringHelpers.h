#ifndef __LIBHPKG_STRINGHELPERS_H__
#define __LIBHPKG_STRINGHELPERS_H__

#include <string>

namespace LibHpkg::Helpers
{
    std::string trim(const std::string& input);
    std::string tolower(const std::string& input);
}

#endif