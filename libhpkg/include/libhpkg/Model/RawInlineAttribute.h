#ifndef __LIBHPKG_MODEL_RAWINLINEATTRIBUTE_H__
#define __LIBHPKG_MODEL_RAWINLINEATTRIBUTE_H__

#include <any>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include "../Compat/ByteSource.h"
#include "AttributeId.h"
#include "RawAttribute.h"

namespace LibHpkg::Model
{
    class RawInlineAttribute: public RawAttribute
    {
    private:
        const std::vector<uint8_t> rawValue;

    public:
        RawInlineAttribute(const AttributeId& attributeId, const std::vector<uint8_t>& rawValue)
            : RawAttribute(attributeId), rawValue(rawValue)
        {
        }

        bool operator==(const RawInlineAttribute& other) const
        {
            return rawValue == other.rawValue;
        }

        virtual std::any GetValue(const AttributeContext& context) const override
        {
            return Compat::ByteSource::Wrap(rawValue);
        }

        AttributeType GetAttributeType() const override 
        {
            return AttributeType::RAW;
        }

        virtual std::string ToString() const override
        {
            return RawAttribute::ToString() + " : " + std::to_string(rawValue.size()) + " b";
        }
    };
}

#endif // __LIBHPKG_MODEL_RAWINLINEATTRIBUTE_H__