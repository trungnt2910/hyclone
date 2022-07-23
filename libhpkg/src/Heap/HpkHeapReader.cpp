#include <cassert>
#include <limits>
#include <string>
#include <type_traits>

#include <magic_enum.hpp>
#include <zlib.h>

#include <libhpkg/Heap/HpkHeapReader.h>
#include <libhpkg/HpkException.h>

namespace LibHpkg::Heap
{
    size_t Inflate(const void *src, size_t srcLen, void *dst, size_t dstLen, bool* needsDictionary = nullptr);

    HpkHeapReader::HpkHeapReader(
                const std::filesystem::path& file,
                HeapCompression compression,
                int64_t heapOffset,
                int64_t chunkSize,
                int64_t compressedSize,
                int64_t uncompressedSize)
        : compression(compression)
        , heapOffset(heapOffset)
        , chunkSize(chunkSize)
        , compressedSize(compressedSize)
        , uncompressedSize(uncompressedSize)
    {
        assert(heapOffset > 0 && heapOffset < std::numeric_limits<int32_t>::max());
        assert(chunkSize > 0 && chunkSize < std::numeric_limits<int32_t>::max());
        assert(compressedSize >= 0 && compressedSize < std::numeric_limits<int32_t>::max());
        assert(uncompressedSize >= 0 && uncompressedSize < std::numeric_limits<int32_t>::max());

        try
        {
            randomAccessFile = std::ifstream(file, std::ios::binary | std::ios::in);
            heapChunkCompressedLengths = std::vector<int>(GetHeapChunkCount());
            PopulateChunkCompressedLengths(heapChunkCompressedLengths);
            heapChunkUncompressedCache.resize(heapChunkCompressedLengths.size());
        }
        catch (const std::exception& e)
        {
            throw HpkException("unable to configure the hpk heap reader", e);
        }
        catch (...)
        {
            throw HpkException("unable to configure the hpk heap reader");
        }
    }

    HpkHeapReader::~HpkHeapReader()
    {
        randomAccessFile.close();
    }

    void HpkHeapReader::Close()
    {
        randomAccessFile.close();
    }

    int HpkHeapReader::GetHeapChunkCount() const
    {
        int count = (int)(uncompressedSize / chunkSize);

        if (uncompressedSize % chunkSize != 0)
        {
            count++;
        }

        return count;
    }

    int HpkHeapReader::GetHeapChunkUncompressedLength(int index) const
    {
        if (index < GetHeapChunkCount() - 1)
        {
            return (int)chunkSize;
        }

        return (int)(uncompressedSize - (chunkSize * (GetHeapChunkCount() - 1)));
    }

    int HpkHeapReader::GetHeapChunkCompressedLength(int index) const
    {
        return heapChunkCompressedLengths[index];
    }

    void HpkHeapReader::PopulateChunkCompressedLengths(std::vector<int>& lengths)
    {
        int count = GetHeapChunkCount();
        int64_t totalCompressedLength = 0;
        randomAccessFile.seekg(heapOffset + compressedSize - (2 * ((int64_t)count - 1)));

        for (int i = 0; i < count - 1; i++)
        {
            // C++ code says that the stored size is length of chunk -1.
            lengths[i] = fileHelper.ReadUnsignedShortToInt(randomAccessFile) + 1;

            if (lengths[i] > uncompressedSize)
            {
                throw HpkException(
                    "the chunk at " + std::to_string(i) +
                    " is of size " + std::to_string(lengths[i]) +
                    ", but the uncompressed length of the chunks is " +
                    std::to_string(uncompressedSize));
            }

            totalCompressedLength += lengths[i];
        }

        // the last one will be missing will need to be derived
        lengths[count - 1] = (int)(compressedSize - ((2 * (count - 1)) + totalCompressedLength));

        if (lengths[count - 1] < 0 || lengths[count - 1] > uncompressedSize)
        {
            throw HpkException(
                "the derivation of the last chunk size of " +
                std::to_string(lengths[count - 1]) +
                " is out of bounds");
        }
    }

    int64_t HpkHeapReader::GetHeapChunkAbsoluteFileOffset(int index) const
    {
        int64_t result = heapOffset; // heap comes after the header.

        for (int i = 0; i < index; i++)
        {
            result += GetHeapChunkCompressedLength(i);
        }

        return result;
    }

    void HpkHeapReader::ReadFully(std::vector<uint8_t>& buffer)
    {
        size_t total = 0;

        while (total < buffer.size())
        {
            randomAccessFile.read(reinterpret_cast<char*>(buffer.data() + total), buffer.size() - total);
            std::streamsize read = randomAccessFile.gcount();

            if (read == 0 && !randomAccessFile.good())
            {
                throw HpkException("unexpected end of file when reading a chunk");
            }

            total += read;
        }
    }

    void HpkHeapReader::ReadHeapChunk(int index, std::vector<uint8_t>& buffer)
    {
        randomAccessFile.seekg(GetHeapChunkAbsoluteFileOffset(index));
        int chunkUncompressedLength = GetHeapChunkUncompressedLength(index);

        if (IsHeapChunkCompressed(index) || HeapCompression::NONE == compression)
        {
            switch (compression)
            {
                case HeapCompression::NONE:
                    throw std::runtime_error("Compressed chunk without compression.");

                case HeapCompression::ZLIB:
                    {
                        std::vector<uint8_t> deflatedBuffer(GetHeapChunkCompressedLength(index));
                        ReadFully(deflatedBuffer);

                        // To-Do: Any C++ equivalents for the commented blocks?
                        // try
                        // {
                            int read;
                            bool needsDictionary = false;

                            if (chunkUncompressedLength != 
                                (read = Inflate(deflatedBuffer.data(), deflatedBuffer.size(), buffer.data(), buffer.size(), &needsDictionary)))
                            {
                                // the last chunk size uncompressed may be smaller than the chunk size,
                                // so don't throw an exception if this happens.
                                if (index < GetHeapChunkCount() - 1)
                                {
                                    std::string message = "a compressed heap chunk inflated to "
                                        + std::to_string(read)
                                        + " bytes; was expecting "
                                        + std::to_string(chunkUncompressedLength);

                                    // if (inflater.NeedsInput())
                                    // {
                                    //     message += "; needs input";
                                    // }

                                    if (needsDictionary)
                                    {
                                        message += "; needs dictionary";
                                    }

                                    throw new HpkException(message);
                                }
                            }

                            // if (!inflater.Finished())
                            // {
                            //     throw new HpkException(string.Format("incomplete inflation of input data while reading chunk {0}", index));
                            // }
                        // }
                        // catch (InvalidDataException dfe)
                        // {
                        //     throw new HpkException("unable to inflate (decompress) heap chunk " + index, dfe);
                        // }
                    }
                    break;

                default:
                    throw std::invalid_argument("unsupported compression; " + std::string(magic_enum::enum_name(compression)));
            }
        }
        else
        {
            randomAccessFile.read((char*)buffer.data(), chunkUncompressedLength);
            int read = randomAccessFile.gcount();

            if (chunkUncompressedLength != read)
            {
                throw HpkException(
                    "problem reading chunk " 
                    + std::to_string(index) 
                    + " of heap; only read "
                    + std::to_string(read)
                    + " of "
                    + std::to_string(buffer.size())
                    + " bytes");
            }
        }
    }

    size_t Inflate(const void *src, size_t srcLen, void *dst, size_t dstLen, bool* needsDictionary)
    {
        z_stream strm  = {0};
        strm.total_in  = strm.avail_in  = srcLen;
        strm.total_out = strm.avail_out = dstLen;
        strm.next_in   = (Bytef *)src;
        strm.next_out  = (Bytef *)dst;

        strm.zalloc = Z_NULL;
        strm.zfree  = Z_NULL;
        strm.opaque = Z_NULL;

        int err = -1;
        size_t ret = -1;

        err = inflateInit2(&strm, 15); //15 window bits, and the +32 tells zlib to to detect if using gzip or zlib
        if (err == Z_OK) 
        {
            err = inflate(&strm, Z_FINISH);
            if (err == Z_STREAM_END) 
            {
                ret = strm.total_out;
            }
            else 
            {
                goto fail;
            }
        }
        else 
        {
        fail:
            inflateEnd(&strm);
            switch (err)
            {
                // Some known errors for compatibility with C# port.
                case Z_NEED_DICT:
                    if (needsDictionary)
                    {
                        *needsDictionary = true;
                    }
                    return 0;
                default:
                    throw HpkException("zlib inflation failed with error: " + std::to_string(err));
            }
        }

        inflateEnd(&strm);
        assert(ret >= 0);
        return ret;
    }

    int HpkHeapReader::ReadHeap(size_t offset)
    {
        assert(offset < uncompressedSize);

        int chunkIndex = (int)(offset / chunkSize);
        int chunkOffset = (int)(offset - (chunkIndex * chunkSize));

        std::optional<std::vector<uint8_t>>& chunkData = heapChunkUncompressedCache[chunkIndex];

        if (chunkData == std::nullopt)
        {
            chunkData = std::vector<uint8_t>(GetHeapChunkUncompressedLength(chunkIndex));
            ReadHeapChunk(chunkIndex, chunkData.value());
        }

        return chunkData.value()[chunkOffset];
    }

    void HpkHeapReader::ReadHeap(std::vector<uint8_t>& buffer, size_t bufferOffset, const HeapCoordinates& coordinates)
    {
        assert(bufferOffset >= 0);
        assert(bufferOffset < buffer.size());
        assert(coordinates.GetOffset() >= 0);
        assert(coordinates.GetOffset() < uncompressedSize);
        assert(coordinates.GetOffset() + coordinates.GetLength() < uncompressedSize);

        // first figure out how much to read from this chunk

        int chunkIndex = (int)(coordinates.GetOffset() / chunkSize);
        int chunkOffset = (int)(coordinates.GetOffset() - (chunkIndex * chunkSize));
        int chunkLength;
        int chunkUncompressedLength = GetHeapChunkUncompressedLength(chunkIndex);

        if (chunkOffset + coordinates.GetLength() > chunkUncompressedLength)
        {
            chunkLength = (chunkUncompressedLength - chunkOffset);
        }
        else
        {
            chunkLength = (int)coordinates.GetLength();
        }

        // now read it in.

        std::optional<std::vector<uint8_t>> chunkData = heapChunkUncompressedCache[chunkIndex];

        if (chunkData == std::nullopt)
        {
            chunkData = std::vector<uint8_t>(GetHeapChunkUncompressedLength(chunkIndex));
            ReadHeapChunk(chunkIndex, chunkData.value());
        }

        std::copy(chunkData->begin() + chunkOffset, chunkData->begin() + chunkOffset + chunkLength, buffer.begin() + bufferOffset);

        // if we need to get some more data from the next chunk then call again.
        // TODO - recursive approach may not be too good when more data is involved; probably ok for hpkr though.

        if (chunkLength < coordinates.GetLength())
        {
            ReadHeap(
                    buffer,
                    bufferOffset + chunkLength,
                    HeapCoordinates(
                            coordinates.GetOffset() + chunkLength,
                            coordinates.GetLength() - chunkLength));
        }
    }
}