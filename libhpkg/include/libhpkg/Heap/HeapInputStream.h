#ifndef __LIBHPKG_HEAP_HEAPINPUTSTREAM_H__
#define __LIBHPKG_HEAP_HEAPINPUTSTREAM_H__

#include <cassert>
#include <iostream>
#include <memory>

#include "HeapCoordinates.h"
#include "HeapReader.h"

namespace LibHpkg::Heap
{
    class heapstreambuf: public std::streambuf
    {
    private:
        std::shared_ptr<HeapReader> reader;
        const HeapCoordinates coordinates;
        size_t offsetInCoordinates = 0;

    public:
        heapstreambuf(const std::shared_ptr<HeapReader>& reader, const HeapCoordinates& coordinates)
            : reader(reader), coordinates(coordinates)
        {
            assert(reader);
        }
        heapstreambuf(const heapstreambuf& other)
            : reader(other.reader), coordinates(other.coordinates), offsetInCoordinates(other.offsetInCoordinates)
        {
        }
        heapstreambuf(heapstreambuf&& other)
            : reader(std::move(other.reader)), coordinates(other.coordinates), offsetInCoordinates(other.offsetInCoordinates)
        {
        }
    protected:
        virtual int sync() override
        {
            return 0;
        }
        virtual int_type underflow() override;
        virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode mode) override;
        virtual pos_type seekpos(pos_type pos, std::ios_base::openmode mode) override;
    };

    class heapistream: public std::istream
    {
    private:
        heapstreambuf streambuf;
    public:
        heapistream(const std::shared_ptr<HeapReader>& reader, const HeapCoordinates& coordinates)
            : streambuf(reader, coordinates), std::istream(&streambuf)
        {
        }
        heapistream(const heapistream& other)
            : streambuf(other.streambuf), std::istream(&streambuf)
        {
        }
        heapistream(heapistream&& other)
            : streambuf(std::move(other.streambuf)), std::istream(&streambuf)
        {
        }
    };

    typedef heapistream HeapInputStream;
}

#endif // __LIBHPKG_HEAP_HEAPINPUTSTREAM_H__