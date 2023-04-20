#ifndef __LIBHPKG_ATTRIBUTE_H__
#define __LIBHPKG_ATTRIBUTE_H__

#include <any>
#include <memory>
#include <vector>

#include "../AttributeContext.h"
#include "AttributeId.h"

namespace LibHpkg::Model
{
    /// <summary>
    /// This is the superclass of the different types (data types) of attributes.
    /// </summary>
    class Attribute
    {
    private:
        const AttributeId attributeId;
        std::vector<std::shared_ptr<Attribute>> childAttributes;
    public:
        Attribute(const AttributeId& attributeId_)
            : attributeId(attributeId_)
        {
        }

        const AttributeId& GetAttributeId() const
        {
            return attributeId;
        }

        virtual AttributeType GetAttributeType() const = 0;

        virtual std::any GetValue(const AttributeContext& context) const = 0;

        std::any GetValue(const std::shared_ptr<AttributeContext>& context) const
        {
            return GetValue(*context);
        }

        template <typename T>
        T GetValue(const AttributeContext& context) const
        {
            return std::any_cast<T>(GetValue(context));
        }

        template <typename T>
        T GetValue(const std::shared_ptr<AttributeContext>& context) const
        {
            return std::any_cast<T>(GetValue(*context));
        }

        void SetChildAttributes(const std::vector<std::shared_ptr<Attribute>>& value)
        {
            childAttributes = value;
        }

        void SetChildAttributes(std::vector<std::shared_ptr<Attribute>>&& value)
        {
            childAttributes = std::move(value);
        }

        bool HasChildAttributes() const
        {
            return !childAttributes.empty();
        }

        const std::vector<std::shared_ptr<Attribute>>& GetChildAttributes() const
        {
            return childAttributes;
        }

        std::vector<std::shared_ptr<Attribute>>
            GetChildAttributes(const AttributeId& attributeId) const;

        std::shared_ptr<Attribute>
            TryGetChildAttribute(const AttributeId& attributeId) const;

        std::shared_ptr<Attribute> GetChildAttribute(const AttributeId& attributeId) const;

        virtual std::string ToString() const;
    };
}

#endif // __LIBHPKG_ATTRIBUTE_H__