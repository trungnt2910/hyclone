#ifndef __LIBHPKG_FILEATTRIBUTEVALUES_H__
#define __LIBHPKG_FILEATTRIBUTEVALUES_H__

namespace LibHpkg::Model
{
    /// <summary>
    /// <para>The attribute type <see cref="AttributeId.FILE_ATTRIBUTE"/> has a value
    /// which is a string that defines what sort of attribute it is.</para>
    /// </summary>
    class FileAttributesValues
    {
    private:
        const char* const attributeValue;

        FileAttributesValues(const char* attributeValue)
            : attributeValue(attributeValue)
        {
        }
    public:
        static const FileAttributesValues BEOS_ICON;
        const char* GetAttributeValue()
        {
            return attributeValue;
        }
    };
}

#endif // __LIBHPKG_FILEATTRIBUTEVALUES_H__