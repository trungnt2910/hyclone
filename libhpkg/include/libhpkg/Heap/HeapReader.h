#ifndef __LIBHPKG_HEAP_HEAPREADER_H__
#define __LIBHPKG_HEAP_HEAPREADER_H__

#include <vector>

#include "HeapCoordinates.h"

namespace LibHpkg::Heap
{
    /// <summary>
    /// This is an interface for classes that are able to provide data from a block of memory referred to as "the heap".
    /// Concrete sub-classes are able to provide specific implementations that can read from different on-disk files to
    /// provide access to a heap.
    /// </summary>
    class HeapReader
    {
    public:
        /// <summary>
        /// This method reads from the heap (possibly across chunks) the data described in the coordinates attribute. It
        /// writes those bytes into the supplied buffer at the offset supplied.
        /// </summary>
        /// <param name="buffer"></param>
        /// <param name="bufferOffset"></param>
        /// <param name="coordinates"></param>
        virtual void ReadHeap(std::vector<uint8_t>& buffer, size_t bufferOffset, const HeapCoordinates& coordinates) = 0;

        /// <summary>
        /// This method reads a single byte of the heap at the given offset.
        /// </summary>
        /// <param name="offset"></param>
        /// <returns></returns>
        virtual int ReadHeap(size_t offset) = 0;
    };
}

#endif // __LIBHPKG_HEAP_HEAPREADER_H__