#ifndef __LIBHPKG_STRINGTABLE_H__
#define __LIBHPKG_STRINGTABLE_H__

#include <string>

namespace LibHpkg
{
    /// <summary>
    /// The attribute-reading elements of the system need to be able to access a string table. This is interface of
    /// an object which is able to provide those strings.
    /// </summary>
    class StringTable
    {
    public:
        /// <summary>
        /// Given the index supplied, this method should return the corresponding string. It will throw an instance 
        /// of <see cref="HpkException"/> if there is any problems associated with achieving this.
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        virtual std::string GetString(size_t index) = 0;
    };
}

#endif // __LIBHPKG_STRINGTABLE_H__