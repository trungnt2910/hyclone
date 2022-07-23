#ifndef __LIBHPKG_ATTRIBUTECONTEXT_H__
#define __LIBHPKG_ATTRIBUTECONTEXT_H__

#include <memory>

namespace LibHpkg
{
    namespace Heap
    {
        class HeapReader;
    }
    
    class StringTable;

    /// <summary>
    /// This object carries around pointers to other data structures and model objects that are required to
    /// support the processing of attributes.
    /// </summary>
    struct AttributeContext
    {
        std::shared_ptr<class Heap::HeapReader> HeapReader;
        std::shared_ptr<class StringTable> StringTable;
    };
} // namespace LibHpkg

#endif // __LIBHPKG_ATTRIBUTE_H__