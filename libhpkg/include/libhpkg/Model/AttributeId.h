#ifndef __LIBHPKG_ATTRIBUTEID_H__
#define __LIBHPKG_ATTRIBUTEID_H__

#include <vector>

#include "AttributeType.h"

namespace LibHpkg::Model
{
    /// <summary>
    /// <para>These constants define the meaning of an <see cref="Attribute"/>.  The numerical value is a value that comes up
    /// in the formatted file that maps to these constants. The string value is a name for the attribute and the type 
    /// gives the type that is expected to be associated with an <see cref="Attribute"/> that has one of these IDs.</para> 
    /// 
    /// <para>These constants were obtained from 
    /// <a href="http://cgit.haiku-os.org/haiku/tree/headers/os/package/hpkg/PackageAttributes.h">here</a> and then the
    /// search/replace of <c>B_DEFINE_HPKG_ATTRIBUTE\([ ]*(\d+),[ ]*([A-Z]+),[ \t]*("[a-z:\-.]+"),[ \t]*([A-Z_]+)\)</c>
    /// <c>$4($1,$3,$2),</c> was applied.</para>
    /// 
    /// <para>For the C# port, the codes were obtained from the Java version using the search/replace of
    /// <c>([A-Z_]*)\((.*)\)[,;]</c> <c>public static readonly AttributeId $1 = new AttributeId($2);</c></para>
    ///
    /// <para>The C++ port uses the regex <c>public static readonly AttributeId ([A-Z_]*) = new ([^;]*);</c>
    /// to search/replace from the C# port.</para>
    ///
    /// </summary>
    class AttributeId
    {
    private:
        static std::vector<AttributeId> _values;
    public:
        static const AttributeId DIRECTORY_ENTRY;
        static const AttributeId FILE_TYPE;
        static const AttributeId FILE_PERMISSIONS;
        static const AttributeId FILE_USER;
        static const AttributeId FILE_GROUP;
        static const AttributeId FILE_ATIME;
        static const AttributeId FILE_MTIME;
        static const AttributeId FILE_CRTIME;
        static const AttributeId FILE_ATIME_NANOS;
        static const AttributeId FILE_MTIME_NANOS;
        static const AttributeId FILE_CRTIM_NANOS;
        static const AttributeId FILE_ATTRIBUTE;
        static const AttributeId FILE_ATTRIBUTE_TYPE;
        static const AttributeId DATA;
        static const AttributeId SYMLINK_PATH;
        static const AttributeId PACKAGE_NAME;
        static const AttributeId PACKAGE_SUMMARY;
        static const AttributeId PACKAGE_DESCRIPTION;
        static const AttributeId PACKAGE_VENDOR;
        static const AttributeId PACKAGE_PACKAGER;
        static const AttributeId PACKAGE_FLAGS;
        static const AttributeId PACKAGE_ARCHITECTURE;
        static const AttributeId PACKAGE_VERSION_MAJOR;
        static const AttributeId PACKAGE_VERSION_MINOR;
        static const AttributeId PACKAGE_VERSION_MICRO;
        static const AttributeId PACKAGE_VERSION_REVISION;
        static const AttributeId PACKAGE_COPYRIGHT;
        static const AttributeId PACKAGE_LICENSE;
        static const AttributeId PACKAGE_PROVIDES;
        static const AttributeId PACKAGE_REQUIRES;
        static const AttributeId PACKAGE_SUPPLEMENTS;
        static const AttributeId PACKAGE_CONFLICTS;
        static const AttributeId PACKAGE_FRESHENS;
        static const AttributeId PACKAGE_REPLACES;
        static const AttributeId PACKAGE_RESOLVABLE_OPERATOR;
        static const AttributeId PACKAGE_CHECKSUM;
        static const AttributeId PACKAGE_VERSION_PRE_RELEASE;
        static const AttributeId PACKAGE_PROVIDES_COMPATIBLE;
        static const AttributeId PACKAGE_URL;
        static const AttributeId PACKAGE_SOURCE_URL;
        static const AttributeId PACKAGE_INSTALL_PATH;
        static const AttributeId PACKAGE_BASE_PACKAGE;
        static const AttributeId PACKAGE_GLOBAL_WRITABLE_FILE;
        static const AttributeId PACKAGE_USER_SETTINGS_FILE;
        static const AttributeId PACKAGE_WRITABLE_FILE_UPDATE_TYPE;
        static const AttributeId PACKAGE_SETTINGS_FILE_TEMPLATE;
        static const AttributeId PACKAGE_USER;
        static const AttributeId PACKAGE_USER_REAL_NAME;
        static const AttributeId PACKAGE_USER_HOME;
        static const AttributeId PACKAGE_USER_SHELL;
        static const AttributeId PACKAGE_USER_GROUP;
        static const AttributeId PACKAGE_GROUP;
        static const AttributeId PACKAGE_POST_INSTALL_SCRIPT;
        static const AttributeId PACKAGE_IS_WRITABLE_DIRECTORY;
        static const AttributeId PACKAGE;
        static const AttributeId PACKAGE_PRE_UNINSTALL_SCRIPT;

    private:
        const int code;
        const char* const name;
        const AttributeType attributeType;
        
        AttributeId(int code, const char* name, AttributeType attributeType)
            : code(code), name(name), attributeType(attributeType)
        {
            _values.push_back(*this);
        }
    
    public:
        AttributeId(const AttributeId& other) = default;
        int GetCode() const { return code; }
        const char* GetName() const { return name; }
        AttributeType GetAttributeType() const { return attributeType; }
        static const std::vector<AttributeId>& GetValues() { return _values; }
        bool operator==(const AttributeId& other) const { return code == other.code; }
    };
}

#endif // __LIBHPKG_ATTRIBUTEID_H__