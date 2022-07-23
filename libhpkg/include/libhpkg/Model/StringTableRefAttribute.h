#ifndef __LIBHPKG_MODEL_STRINGTABLEREFATTRIBUTE_H__
#define __LIBHPKG_MODEL_STRINGTABLEREFATTRIBUTE_H__

#include <any>
#include <cassert>
#include <string>

#include "StringAttribute.h"
#include "../StringTable.h"

namespace LibHpkg::Model
{
    /// <summary>
    /// This type of attribute references a string from an instance of <see cref="HpkStringTable"/>
    /// which is typically obtained from an instance of <see cref="AttributeContext"/>.
    /// </summary>
    class StringTableRefAttribute: public StringAttribute
    {
    private:
        const int index;
    
    public:
        StringTableRefAttribute(const AttributeId& attributeId, int index)
            : StringAttribute(attributeId), index(index)
        {
            assert(!"bad index" || index >= 0);
        }

        virtual std::any GetValue(const AttributeContext& context) const override
        {
            return context.StringTable->GetString(index);
        }

        virtual AttributeType GetAttributeType() const override
        {
            return AttributeType::STRING;
        }

        bool operator==(const StringTableRefAttribute& other) const
        {
            return index == other.index;
        }

        virtual std::string ToString() const override
        {
            return StringAttribute::ToString() + " : @" + std::to_string(index);
        }
    };
}


#endif // __LIBHPKG_MODEL_STRINGTABLEREFATTRIBUTE_H__