#ifndef __LIBHPKG_HEAPCOMPRESSION_H__
#define __LIBHPKG_HEAPCOMPRESSION_H__

namespace LibHpkg::Heap
{
    enum class HeapCompression
    {
        NONE = 0,
        ZLIB = 1,
        ZSTD = 2,
    };
}

#endif // __LIBHPKG_HEAPCOMPRESSION_H__