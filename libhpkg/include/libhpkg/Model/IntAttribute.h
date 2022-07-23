#ifndef __LIBHPKG_INTATTRIBUTE_H__
#define __LIBHPKG_INTATTRIBUTE_H__

#include <gmpxx.h>

#include "Attribute.h"

namespace LibHpkg::Model
{
    /// <summary>
    /// This attribute is an integral numeric value. Note that the format specifies either a signed or unsigned value,
    /// but this concrete subclass of @{link Attribute} serves for both the signed and unsigned cases.
    /// </summary>
    class IntAttribute : public Attribute
    {
    private:
        const mpz_class numericValue;
    
    public:
        IntAttribute(const AttributeId& AttributeId, const mpz_class& numericValue)
            : Attribute(AttributeId), numericValue(numericValue)
        {
        }

        std::any GetValue(const AttributeContext& context) const override
        {
            return numericValue;
        }

        AttributeType GetAttributeType() const override
        {
            return AttributeType::INT;
        }

        bool operator==(const IntAttribute& other) const
        {
            return numericValue == other.numericValue;
        }

        std::string ToString() const override
        {
            return Attribute::ToString() + " : " + numericValue.get_str();
        }
    };

    // TODO: GetHashCode() port using std::hash
}

#endif // __LIBHPKG_INTATTRIBUTE_H__