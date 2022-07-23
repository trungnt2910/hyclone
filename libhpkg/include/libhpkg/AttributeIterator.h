#ifndef __LIBHPKG_ATTRIBUTE_ITERATOR_H__
#define __LIBHPKG_ATTRIBUTE_ITERATOR_H__

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>

#include <gmpxx.h>

#include "AttributeContext.h"
#include "Model/Attribute.h"
#include "Model/AttributeId.h"

namespace LibHpkg
{
    /// <summary>
    /// <para>This object is able to provide an iterator through all of the attributes at a given offset in the chunks. The
    /// chunk data is supplied through an instance of <see cref="AttributeContext"/>. It will work through all of the 
    /// attributes serially and will also process all of the child-attributes as well. The iteration process means that
    /// less in-memory data is required to process a relatively long list of attributes.</para>
    ///  
    /// <para>Use the method <see cref="HasNext"/> to find out if there is another attribute to read and <see cref="Next"/> in 
    /// order to obtain the next attribute.</para>
    /// 
    /// <para>Note that this does not actually implement <see cref="Iterator"/> because it needs to throw Hpk exceptions
    /// which would mean that it were not compliant with the @{link Iterator} interface.</para>
    /// </summary>
    class AttributeIterator
    {
    private:
        static const int ATTRIBUTE_TYPE_INVALID = 0;
        static const int ATTRIBUTE_TYPE_INT = 1;
        static const int ATTRIBUTE_TYPE_UINT = 2;
        static const int ATTRIBUTE_TYPE_STRING = 3;
        static const int ATTRIBUTE_TYPE_RAW = 4;

        static const int ATTRIBUTE_ENCODING_INT_8_BIT = 0;
        static const int ATTRIBUTE_ENCODING_INT_16_BIT = 1;
        static const int ATTRIBUTE_ENCODING_INT_32_BIT = 2;
        static const int ATTRIBUTE_ENCODING_INT_64_BIT = 3;

        static const int ATTRIBUTE_ENCODING_STRING_INLINE = 0;
        static const int ATTRIBUTE_ENCODING_STRING_TABLE = 1;

        static const int ATTRIBUTE_ENCODING_RAW_INLINE = 0;
        static const int ATTRIBUTE_ENCODING_RAW_HEAP = 1;

        size_t offset;

        const std::shared_ptr<AttributeContext> context;

        std::optional<mpz_class> nextTag = std::nullopt;

    public:
        AttributeIterator(const std::shared_ptr<AttributeContext>& context, size_t offset)
            : offset(offset), context(context)
        {
            assert(context);
            assert(offset >= 0 && offset < std::numeric_limits<int>::max());
        }

        const std::shared_ptr<AttributeContext>& GetContext() const
        {
            return context;
        }

        size_t GetOffset() const
        {
            return offset;
        }

        /// <summary>
        /// This method allows the caller to discover if there is another attribute to get off the iterator.
        /// </summary>
        /// <returns></returns>
        bool HasNext()
        {
            return 0 != sgn(GetNextTag());
        }

        /// <summary>
        /// This method will return the next <see cref="Model::Attribute"/>. If there is not another value to return then
        /// this method will return null. It will throw an instance of @{link HpkException} in any situation in which
        /// it is not able to parse the data or chunks such that it is not able to read the next attribute.
        /// </summary>
        /// <returns></returns>
        /// <exception cref="HpkException"></exception>
        std::shared_ptr<Model::Attribute> Next();

    private:
        std::vector<std::shared_ptr<Model::Attribute>> ReadChildAttributes();

        /**
         * each attribute id has a type associated with it; now check that the attribute matches
         * its intended type.
         */

        void EnsureAttributeType(const std::shared_ptr<Model::Attribute>& attribute);

        std::shared_ptr<Model::Attribute> ReadAttributeByTagType(int tagType, int encoding, const Model::AttributeId& attributeId);

        std::vector<uint8_t> ReadBufferForInt(int encoding);

        std::shared_ptr<Model::Attribute> ReadString(int encoding, const Model::AttributeId& attributeId);

        std::shared_ptr<Model::Attribute> ReadStringTable(const Model::AttributeId& attributeId);

        std::shared_ptr<Model::Attribute> ReadStringInline(const Model::AttributeId& attributeId);

        std::shared_ptr<Model::Attribute> ReadRaw(int encoding, const Model::AttributeId& attributeId);

        std::shared_ptr<Model::Attribute> ReadRawInline(const Model::AttributeId& attributeId);

        std::shared_ptr<Model::Attribute> ReadRawHeap(const Model::AttributeId& attributeId);
        
        int DeriveAttributeTagType(const mpz_class& tag)
        {
            mpz_class result;
            (((tag - 1) >> 7) & 0x7L).eval(result.get_mpz_t());
            return result.get_si();
        }

        int DeriveAttributeTagId(const mpz_class& tag)
        {
            mpz_class result;
            ((tag - 1) & 0x7F).eval(result.get_mpz_t());
            return result.get_si();
        }

        int DeriveAttributeTagEncoding(const mpz_class& tag)
        {
            mpz_class result;
            (((tag - 1) >> 11) & 0x3L).eval(result.get_mpz_t());
            return result.get_si();
        }

        bool DeriveAttributeTagHasChildAttributes(const mpz_class& tag)
        {
            mpz_class result;
            (((tag - 1) >> 10) & 0x1L).eval(result.get_mpz_t());
            return 0 != result.get_si();
        }

        const mpz_class& GetNextTag()
        {
            if (std::nullopt == nextTag)
            {
                nextTag = ReadUnsignedLeb128();
            }

            return nextTag.value();
        }

        mpz_class ReadUnsignedLeb128();

        void EnsureValidEncodingForInt(int encoding);
    };
}

#endif // __LIBHPKG_ATTRIBUTE_ITERATOR_H__