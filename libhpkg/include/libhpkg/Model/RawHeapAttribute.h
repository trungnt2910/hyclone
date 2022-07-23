#ifndef __LIBHPKG_MODEL_RAWHEAPATTRIBUTE_H__
#define __LIBHPKG_MODEL_RAWHEAPATTRIBUTE_H__

#include <any>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>

#include "../Compat/ByteSource.h"
#include "../Heap/HeapCoordinates.h"
#include "../Heap/HeapInputStream.h"
#include "../Heap/HeapReader.h"
#include "RawAttribute.h"

namespace LibHpkg::Model
{
    class HeapByteSource: public Compat::ByteSource
    {
    private:
        const std::shared_ptr<Heap::HeapReader> heapReader;
        const Heap::HeapCoordinates heapCoordinates;

    public:
        HeapByteSource(const std::shared_ptr<Heap::HeapReader>& heapReader, const Heap::HeapCoordinates& heapCoordinates)
            : heapReader(heapReader), heapCoordinates(heapCoordinates)
        {
        }

        virtual std::shared_ptr<std::istream> OpenStream() const override
        {
            return std::make_shared<Heap::HeapInputStream>(heapReader, heapCoordinates);
        }

        virtual std::optional<size_t> SizeIfKnown() const override
        {
            return heapCoordinates.GetLength();
        }

        const Heap::HeapCoordinates& GetHeapCoordinates() const
        {
            return heapCoordinates;
        }
    };

    class RawHeapAttribute: public RawAttribute
    {
        friend class std::hash<RawHeapAttribute>;
    private:
        const Heap::HeapCoordinates heapCoordinates;

    public:
        RawHeapAttribute(const AttributeId& attributeId, const Heap::HeapCoordinates& heapCoordinates)
            : RawAttribute(attributeId), heapCoordinates(heapCoordinates)
        {
        }

        bool operator==(const RawHeapAttribute& other) const
        {
            return heapCoordinates == other.heapCoordinates;
        }

        virtual std::any GetValue(const AttributeContext& context) const override
        {
            return std::dynamic_pointer_cast<Compat::ByteSource>(
                std::make_shared<HeapByteSource>(context.HeapReader, heapCoordinates));
        }

        virtual AttributeType GetAttributeType() const override
        {
            return AttributeType::RAW;
        }

        virtual std::string ToString() const override
        {
            return RawAttribute::ToString() + " : @" + heapCoordinates.ToString();
        }

        const Heap::HeapCoordinates& GetHeapCoordinates() const
        {
            return heapCoordinates;
        }
    };
}

namespace std
{
    template <>
    struct hash<LibHpkg::Model::RawHeapAttribute>
    {
        size_t operator()(const LibHpkg::Model::RawHeapAttribute& rawHeapAttribute) const
        {
            return hash<LibHpkg::Heap::HeapCoordinates>()(rawHeapAttribute.heapCoordinates);
        }
    };
}

#endif // __LIBHPKG_MODEL_RAWHEAPATTRIBUTE_H__