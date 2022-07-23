#include <string>
#include <vector>

#include <libhpkg/HpkException.h>
#include <libhpkg/HpkStringTable.h>

namespace LibHpkg
{
    using namespace LibHpkg::Heap;

    std::vector<std::string> HpkStringTable::ReadStrings() const
    {
        std::vector<std::string> result(expectedCount);
        std::vector<uint8_t> stringsDataBuffer(heapLength);;

        heapReader->ReadHeap(stringsDataBuffer, 0,
            HeapCoordinates(heapOffset, heapLength));

        // now work through the data and load them into the strings.

        int stringIndex = 0;
        int offset = 0;

        while (offset < stringsDataBuffer.size())
        {

            if (0 == stringsDataBuffer[offset])
            {
                if (stringIndex != result.size())
                {
                    throw HpkException(
                        "expected to read "
                        + std::to_string(expectedCount)
                        + " package strings from the strings table, but actually found "
                        + std::to_string(stringIndex));
                }

                return result;
            }

            if (stringIndex >= expectedCount)
            {
                throw HpkException("have already read all of the strings from the string table, but have not exhausted the string table data");
            }

            int start = offset;

            while (offset < stringsDataBuffer.size() && 0 != stringsDataBuffer[offset])
            {
                offset++;
            }

            if (offset < stringsDataBuffer.size())
            {
                result[stringIndex] = std::string((const char*)stringsDataBuffer.data() + start, offset - start);
                stringIndex++;
                offset++;
            }
        }

        throw HpkException(
            "expected to find the null-terminator for the list of strings, but was not able to find one; did read " 
            + std::to_string(stringIndex) 
            + " of " 
            + std::to_string(expectedCount));
    }

}