#include <algorithm>
#include <stdexcept>

#include <magic_enum.hpp>

#include <libhpkg/Model/Attribute.h>

namespace LibHpkg::Model
{
    std::vector<std::shared_ptr<Attribute>>
        Attribute::GetChildAttributes(const AttributeId& attributeId) const
    {
        std::vector<std::shared_ptr<Attribute>> result;
        for (const auto& a : childAttributes)
        {
            if (a->attributeId == attributeId)
            {
                result.push_back(a);
            }
        }
        return result;
    }

    std::shared_ptr<Attribute>
        Attribute::TryGetChildAttribute(const AttributeId& attributeId) const
    {
        auto it = std::find_if(childAttributes.begin(), childAttributes.end(),
            [&](const std::shared_ptr<Attribute>& a) { return a->attributeId == attributeId; });
        if (it != childAttributes.end())
        {
            return *it;
        }
        return std::shared_ptr<Attribute>();
    }

    std::shared_ptr<Attribute> 
        Attribute::GetChildAttribute(const AttributeId& attributeId) const
    {
        auto val = TryGetChildAttribute(attributeId);
        return val ? val :
            throw std::invalid_argument("unable to find the attribute [" + std::string(attributeId.GetName()) + "]");
    }

    std::string Attribute::ToString() const
    {
        return GetAttributeId().GetName() + std::string(" : ") +
            std::string(magic_enum::enum_name(GetAttributeType()));
    }
}