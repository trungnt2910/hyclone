#ifndef __LIBHPKG_MODEL_PKGVERSION_H__
#define __LIBHPKG_MODEL_PKGVERSION_H__

#include <cassert>
#include <optional>
#include <string>

namespace LibHpkg::Model
{
    class PkgVersion
    {
    private:
        const std::string major;
        const std::string minor;
        const std::string micro;
        const std::string preRelease;
        const std::optional<int> revision;

        void AppendDotValue(std::string& str, const std::string& value) const;
    public:
        PkgVersion(const std::string& major, const std::string& minor, 
            const std::string& micro, const std::string& preRelease, 
            const std::optional<int>& revision)
            : major(major)
            , minor(minor)
            , micro(micro)
            , preRelease(preRelease)
            , revision(revision)
        {
            assert(!major.empty());
        }

        const std::string& GetMajor() const
        {
            return major;
        }
        
        const std::string& GetMinor() const
        {
            return minor;
        }

        const std::string& GetMicro() const
        {
            return micro;
        }

        const std::string& GetPreRelease() const
        {
            return preRelease;
        }

        const std::optional<int>& GetRevision() const
        {
            return revision;
        }

        virtual std::string ToString() const;
    };
}

#endif // __LIBHPKG_MODEL_PKGVERSION_H__