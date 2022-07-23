#ifndef __HEAP_HEAPCOORDINATES_H__
#define __HEAP_HEAPCOORDINATES_H__

#include <cassert>
#include <climits>
#include <cstdint>
#include <functional>
#include <string>

namespace LibHpkg::Heap
{
    /// <summary>
    /// This object provides an offset and length into the heap and this provides a coordinate for a chunk of
    /// data in the heap. Note that the coordinates refer to the uncompressed data across all of the chunks of the heap.
    /// </summary>
    class HeapCoordinates
    {
        friend class std::hash<HeapCoordinates>;
    private:
        size_t offset;
        size_t length;
    public:
        HeapCoordinates(size_t offset, size_t length) 
            : offset(offset), length(length)
        {
            assert(offset >= 0 && offset < INT_MAX);
            assert(length >= 0 && length < INT_MAX);
        }

        size_t GetOffset() const { return offset; }
        size_t GetLength() const { return length; }
        bool IsEmpty() const { return length == 0; }

        bool operator==(const HeapCoordinates& other) const
        {
            return offset == other.offset && length == other.length;
        }

        virtual std::string ToString() const
        {
            return std::to_string(offset) + "," + std::to_string(length);
        }
    };
}

namespace std
{
    template <>
    struct hash<LibHpkg::Heap::HeapCoordinates>
    {
        size_t operator()(const LibHpkg::Heap::HeapCoordinates& c) const
        {
            size_t result = (size_t)(c.offset ^ (c.offset >> 32));
            result = 31 * result + (size_t)(c.length ^ (c.length >> 32));
            return result;
        }
    };
}

#endif // __HEAP_HEAPCOORDINATES_H__