#ifndef __LIBHPKG_MODEL_STRINGATTRIBUTE_H__
#define __LIBHPKG_MODEL_STRINGATTRIBUTE_H__

#include "Attribute.h"

namespace LibHpkg::Model
{
    class StringAttribute: public Attribute
    {
        // While on C# and Java this member is public,
        // the class is still protected from being created
        // by the language as it's abstrace.
        // There is no equal mechanism in C++,
        // so make this member protected.
    protected:
        StringAttribute(const AttributeId& attributeId)
            : Attribute(attributeId)
        {
        }
    };
}

#endif // __LIBHPKG_MODEL_STRINGATTRIBUTE_H__