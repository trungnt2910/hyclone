#ifndef __LIBHPKG_MODEL_PKGURL_H__
#define __LIBHPKG_MODEL_PKGURL_H__

#include <cassert>
#include <cctype>
#include <regex>
#include <stdexcept>
#include <string>

#include "../Helpers/StringHelpers.h"
#include "PkgUrlType.h"

namespace LibHpkg::Model
{
    /// <summary>
    /// <para>A package URL when supplied as a string can be either the URL &quot;naked&quot; or it can be of a form such
    /// as;</para>
    /// <code>Foo Bar &lt;http://www.example.com/&gt;</code>
    /// <para>This class is able to parse those forms and model the resultant name and URL.</para>
    /// </summary>
    class PkgUrl
    {
    private:
        static const std::regex PATTERN_NAMEANDURL;

        std::string url;
        std::string name;
        const PkgUrlType urlType;

    public:
        PkgUrl(const std::string& input, PkgUrlType urlType);

        PkgUrlType GetUrlType() const { return urlType; }

        const std::string& GetUrl() const { return url; }

        const std::string& GetName() const { return name; }

        virtual std::string ToString() const;
    };
}

#endif // __LIBHPKG_MODEL_PKGURL_H__