#ifndef __LIBHPKG_MODEL_STRINGINLINEATTRIBUTE_H__
#define __LIBHPKG_MODEL_STRINGINLINEATTRIBUTE_H__

#include <any>
#include <string>

#include "AttributeType.h"
#include "StringAttribute.h"

namespace LibHpkg::Model
{
    /// <summary>
    /// This type of attribute is a string. The string is supplied in the stream of attributes so this attribute will
    /// carry the string.
    /// </summary>
    class StringInlineAttribute: public StringAttribute
	{
    private:
        const std::string stringValue;

    public:
		StringInlineAttribute(const AttributeId& attributeId, const std::string& stringValue)
			: StringAttribute(attributeId), stringValue(stringValue)
		{
		}

		virtual std::any GetValue(const AttributeContext& context) const override
        {
            return stringValue;
        }

		virtual AttributeType GetAttributeType() const override
		{
			return AttributeType::STRING;
		}

		bool operator==(const StringInlineAttribute& other) const
		{
			return stringValue == other.stringValue;
		}

		virtual std::string ToString() const
		{
			return StringAttribute::ToString() + " : " + stringValue;
		}
	};
}

#endif // __LIBHPKG_MODEL_STRINGINLINEATTRIBUTE_H__