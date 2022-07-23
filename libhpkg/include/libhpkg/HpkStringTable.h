#ifndef __LIBHPKG_HPKSTRINGTABLE_H__
#define __LIBHPKG_HPKSTRINGTABLE_H__

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "Heap/HpkHeapReader.h"
#include "StringTable.h"

namespace LibHpkg
{
    /// <summary>
    /// The HPK* file format may contain a table of commonly used strings in a table.  This object will represent
    /// those strings and will lazily load them from the heap as necessary.
    /// </summary>
    class HpkStringTable: public StringTable
    {
    private:
        const std::shared_ptr<Heap::HpkHeapReader> heapReader;

        const size_t expectedCount;

        const size_t heapLength;

        const size_t heapOffset;

        std::vector<std::string> values;

    public:
        HpkStringTable(
                const std::shared_ptr<Heap::HpkHeapReader>& heapReader,
                size_t heapOffset,
                size_t heapLength,
                size_t expectedCount)
            : StringTable(), heapReader(heapReader), expectedCount(expectedCount), heapLength(heapLength), heapOffset(heapOffset)
        {
            assert(heapReader);
            assert(heapOffset >= 0 && heapOffset < std::numeric_limits<int>::max());
            assert(heapLength >= 0 && heapLength < std::numeric_limits<int>::max());
            assert(expectedCount >= 0 && expectedCount < std::numeric_limits<int>::max());
        }

        // TODO; could avoid the big read into a buffer by reading the heap byte by byte or with a buffer.
    private: 
        std::vector<std::string> ReadStrings() const;

        const std::vector<std::string>& GetStrings()
        {
            if (values.empty() && heapLength > 0)
            {
                values = ReadStrings();
            }

            return values;
        }

        virtual std::string GetString(size_t index) override
        {
            return GetStrings()[index];
        }
    };
}


#endif // __LIBHPKG_HPKSTRINGTABLE_H__