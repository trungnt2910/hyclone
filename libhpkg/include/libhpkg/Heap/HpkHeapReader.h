#ifndef __LIBHPKG_HEAP_HHPKHEAPREADER_H__
#define __LIBHPKG_HEAP_HHPKHEAPREADER_H__

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <vector>

#include "../FileHelper.h"
#include "HeapCompression.h"
#include "HeapReader.h"

namespace LibHpkg::Heap
{
    /**
     * <P>An instance of this class is able to read the heap's chunks that are in HPK format.  Note
     * that this class will also take responsibility for caching the chunks so that a subsequent
     * read from the same chunk will not require a re-fault from disk.</P>
     */
    class HpkHeapReader: public HeapReader
    {
    private:
        const HeapCompression compression;
        const int64_t heapOffset;
        const int64_t chunkSize;
        const int64_t compressedSize; // including the shorts for the chunks' compressed sizes
        const int64_t uncompressedSize; // excluding the shorts for the chunks' compressed sizes

        // private readonly LoadingCache<int, byte[]> heapChunkUncompressedCache;
        // The original Java version uses a LoadingCache, and the .NET port
        // tries to emulate this using a MemoryCache. While this may be useful
        // for server applications (having to load numerous hpkgs at the same
        // time to serve clients), this may not be useful to normal apps
        // like hyclone_server.
        std::vector<std::optional<std::vector<uint8_t>>> heapChunkUncompressedCache;
        std::vector<int> heapChunkCompressedLengths;
        std::ifstream randomAccessFile;

        const FileHelper fileHelper = FileHelper();

    public:
        HpkHeapReader(
                const std::filesystem::path& file,
                HeapCompression compression,
                int64_t heapOffset,
                int64_t chunkSize,
                int64_t compressedSize,
                int64_t uncompressedSize);

        ~HpkHeapReader();

        void Close();

        virtual int ReadHeap(size_t offset) override;

        virtual void ReadHeap(std::vector<uint8_t>& buffer, size_t bufferOffset, const HeapCoordinates& coordinates) override;

    private:
        /// <summary>
        /// This gives the quantity of chunks that are in the heap.
        /// </summary>
        int GetHeapChunkCount() const;

        int GetHeapChunkUncompressedLength(int index) const;

        int GetHeapChunkCompressedLength(int index) const;

        /// <summary>
        /// After the chunk data is a whole lot of unsigned shorts that define the compressed
        /// size of the chunks in the heap.This method will shift the input stream to the
        /// start of those shorts and read them in.
        /// </summary>
        void PopulateChunkCompressedLengths(std::vector<int>& lengths);

        inline bool IsHeapChunkCompressed(int index) const
        {
            return GetHeapChunkCompressedLength(index) < GetHeapChunkUncompressedLength(index);
        }

        int64_t GetHeapChunkAbsoluteFileOffset(int index) const;

        /// <summary>
        /// This will read from the current offset into the supplied buffer until the supplied buffer is completely
        /// filledup.
        /// </summary>
        /// <param name="buffer"></param>
        /// <exception cref="HpkException"></exception>
        void ReadFully(std::vector<uint8_t>& buffer);

        /// <summary>
        /// This will read a chunk of the heap into the supplied buffer. It is assumed that the buffer will be
        /// of the correct length for the uncompressed heap chunk size.
        /// </summary>
        /// <param name="index"></param>
        /// <param name="buffer"></param>
        /// <exception cref="IllegalStateException"></exception>
        /// <exception cref="HpkException"></exception>
        void ReadHeapChunk(int index, std::vector<uint8_t>& buffer);
    };
}

#endif // __LIBHPKG_HEAP_HHPKHEAPREADER_H__