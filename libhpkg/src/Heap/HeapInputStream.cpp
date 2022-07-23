#include <libhpkg/Heap/HeapInputStream.h>

namespace LibHpkg::Heap
{
    heapstreambuf::int_type heapstreambuf::underflow()
    {
        if (offsetInCoordinates >= coordinates.GetLength())
        {
            return traits_type::eof();
        }

        size_t remainingLength = coordinates.GetLength() - offsetInCoordinates;
        size_t readLength = std::min(remainingLength, buffer.size());

        reader->ReadHeap(buffer, 0, HeapCoordinates(coordinates.GetOffset() + offsetInCoordinates, readLength));

        offsetInCoordinates += readLength;

        setg((char*)buffer.data(),
            (char*)buffer.data(),
            (char*)buffer.data() + readLength);

        return buffer[0];
    }

    heapstreambuf::pos_type heapstreambuf::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode mode)
    {
        if (mode & std::ios_base::out)
        {
            return pos_type(-1);
        }

        if (dir == std::ios_base::beg)
        {
            offsetInCoordinates = off;
        }
        else if (dir == std::ios_base::cur)
        {
            offsetInCoordinates += off;
        }
        else if (dir == std::ios_base::end)
        {
            offsetInCoordinates = coordinates.GetLength() + off;
        }
        else
        {
            return pos_type(off_type(-1));
        }

        return pos_type(offsetInCoordinates);
    }

    heapstreambuf::pos_type heapstreambuf::seekpos(pos_type pos, std::ios_base::openmode mode)
    {
        return seekoff(pos, std::ios_base::beg, mode);
    }
} // namespace LibHpkg::Heap
