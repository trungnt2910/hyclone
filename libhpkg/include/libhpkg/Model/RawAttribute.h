#ifndef __LIBHPKG_RAWATTRIBUTE_H__
#define __LIBHPKG_RAWATTRIBUTE_H__

#include "Attribute.h"
#include "AttributeId.h"

namespace LibHpkg::Model
{
	class RawAttribute: public Attribute
	{
	protected:
        RawAttribute(const AttributeId& attributeId)
			: Attribute(attributeId)
		{
		}
	};
}


#endif // __LIBHPKG_RAWATTRIBUTE_H__