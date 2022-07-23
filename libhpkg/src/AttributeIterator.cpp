#include <memory>
#include <sstream>

#include <gmpxx.h>
#include <magic_enum.hpp>

#include <libhpkg/AttributeIterator.h>
#include <libhpkg/Heap/HeapCoordinates.h>
#include <libhpkg/Heap/HeapReader.h>
#include <libhpkg/HpkException.h>
#include <libhpkg/Model/IntAttribute.h>
#include <libhpkg/Model/RawHeapAttribute.h>
#include <libhpkg/Model/RawInlineAttribute.h>
#include <libhpkg/Model/StringInlineAttribute.h>
#include <libhpkg/Model/StringTableRefAttribute.h>

namespace LibHpkg
{
    using namespace LibHpkg::Heap;
    using namespace LibHpkg::Model;
    using BigInteger = mpz_class;

    BigInteger Construct(const std::vector<uint8_t>& bytes)
    {
        BigInteger result;
        mpz_import(
            // mpz_t target
            result.get_mpz_t(),
            // count of buffer
            bytes.size(),
            // order of bytes in buffer
            1,
            // size of each buffer element
            1,
            // endian (use big-endian because that's what the server on Java does)
            1,
            // nails (don't know wtf that is)
            0,
            bytes.data());
        return result;
    }

    BigInteger Construct(int sign, const std::vector<uint8_t>& bytes)
    {
        BigInteger result = Construct(bytes);
        if (sign < 0)
        {
            result = -result;
        }
        return result;
    }

    std::shared_ptr<Attribute> AttributeIterator::Next()
    {
        std::shared_ptr<Attribute> result;

        // first, the LEB128 has to be read in which is the 'tag' defining what sort of attribute this is that
        // we are dealing with.

        const BigInteger& tag = GetNextTag();

        // if we encounter 0 tag then we know that we have finished the list.

        if (0 != sgn(tag))
        {
            int encoding = DeriveAttributeTagEncoding(tag);
            int id = DeriveAttributeTagId(tag);

            if (id < 0 || id >= AttributeId::GetValues().size())
            {
                throw HpkException("illegal id; " + id);
            }
            const AttributeId& attributeId = AttributeId::GetValues()[id];
            result = ReadAttributeByTagType(DeriveAttributeTagType(tag), encoding, attributeId);
            EnsureAttributeType(result);

            if (result->GetAttributeId().GetAttributeType() != result->GetAttributeType())
            {
                throw HpkException(
                        "mismatch in attribute type for id "
                        + std::string(result->GetAttributeId().GetName())
                        + "; expecting "
                        + std::string(
                            magic_enum::enum_name(
                                result->GetAttributeId().GetAttributeType()))
                        + ", but got "
                        + std::string(
                            magic_enum::enum_name(
                                result->GetAttributeType())));
            }

            // possibly there are child attributes after this attribute; if this is the
            // case then open-up a new iterator to work across those and load them in.
            if (DeriveAttributeTagHasChildAttributes(tag))
            {
                result->SetChildAttributes(ReadChildAttributes());
            }

            nextTag = std::nullopt;
        }

        return result;
    }

    std::vector<std::shared_ptr<Attribute>> AttributeIterator::ReadChildAttributes()
    {
        std::vector<std::shared_ptr<Attribute>> children;
        AttributeIterator childAttributeIterator(context, offset);

        while (childAttributeIterator.HasNext())
        {
            children.push_back(childAttributeIterator.Next());
        }

        offset = childAttributeIterator.GetOffset();
        return children;
    }

    void AttributeIterator::EnsureAttributeType(const std::shared_ptr<Attribute>& attribute)
    {
        if (attribute->GetAttributeId().GetAttributeType()
            != attribute->GetAttributeType())
        {
            throw HpkException(
                    "mismatch in attribute type for id "
                    + std::string(attribute->GetAttributeId().GetName())
                    + "; expecting "
                    + std::string(
                        magic_enum::enum_name(
                            attribute->GetAttributeId().GetAttributeType()))
                    + ", but got "
                    + std::string(
                        magic_enum::enum_name(
                            attribute->GetAttributeType())));
        }
    }

    std::shared_ptr<Attribute> AttributeIterator::ReadAttributeByTagType(int tagType, int encoding, const AttributeId& attributeId)
    {
        switch (tagType)
        {
            case ATTRIBUTE_TYPE_INVALID:
                throw HpkException("an invalid attribute tag type has been encountered");
            case ATTRIBUTE_TYPE_INT:
                return std::make_shared<IntAttribute>(attributeId, Construct(ReadBufferForInt(encoding)));
            case ATTRIBUTE_TYPE_UINT:
                return std::make_shared<IntAttribute>(attributeId, Construct(1, ReadBufferForInt(encoding)));
            case ATTRIBUTE_TYPE_STRING:
                return ReadString(encoding, attributeId);
            case ATTRIBUTE_TYPE_RAW:
                return ReadRaw(encoding, attributeId);
            default:
                throw HpkException("unable to read the tag type [" + std::to_string(tagType) + "]");

        }
    }

    std::vector<uint8_t> AttributeIterator::ReadBufferForInt(int encoding)
    {
        EnsureValidEncodingForInt(encoding);
        int bytesToRead = 1 << encoding;
        std::vector<uint8_t> buffer(bytesToRead);
        context->HeapReader->ReadHeap(buffer, 0, HeapCoordinates(offset, bytesToRead));
        offset += bytesToRead;

        return buffer;
    }

    std::shared_ptr<Attribute> AttributeIterator::ReadString(int encoding, const AttributeId& attributeId)
    {
        switch (encoding)
        {
            case ATTRIBUTE_ENCODING_STRING_INLINE:
                return ReadStringInline(attributeId);
            case ATTRIBUTE_ENCODING_STRING_TABLE:
                return ReadStringTable(attributeId);
            default:
                throw HpkException("unknown string encoding; " + encoding);
        }
    }

    std::shared_ptr<Attribute> AttributeIterator::ReadStringTable(const AttributeId& attributeId)
    {
        BigInteger index = ReadUnsignedLeb128();

        if (!index.fits_sint_p())
        {
            throw std::runtime_error("the string table index is preposterously large");
        }

        return std::make_shared<StringTableRefAttribute>(attributeId, index.get_si());
    }

    std::shared_ptr<Attribute> AttributeIterator::ReadStringInline(const AttributeId& attributeId)
    {
        std::string assembly;

        while (true)
        {
            int b = context->HeapReader->ReadHeap(offset);
            offset++;

            if (0 != b)
            {
                assembly.push_back(b);
            }
            else
            {
                return std::make_shared<StringInlineAttribute>(
                        attributeId,
                        assembly);
            }
        }
    }

    std::shared_ptr<Attribute> AttributeIterator::ReadRaw(int encoding, const AttributeId& attributeId)
    {
        switch (encoding)
        {
            case ATTRIBUTE_ENCODING_RAW_INLINE:
                return ReadRawInline(attributeId);
            case ATTRIBUTE_ENCODING_RAW_HEAP:
                return ReadRawHeap(attributeId);
            default:
                throw HpkException("unknown raw encoding; " + encoding);
        }
    }

    std::shared_ptr<Attribute> AttributeIterator::ReadRawInline(const AttributeId& attributeId)
    {
        BigInteger length = ReadUnsignedLeb128();

        if (!length.fits_sint_p())
        {
            throw HpkException("the length of the inline data is too large");
        }

        std::vector<uint8_t> buffer(length.get_si());
        context->HeapReader->ReadHeap(buffer, 0, HeapCoordinates(offset, length.get_si()));
        offset += length.get_si();

        return std::make_shared<RawInlineAttribute>(attributeId, buffer);
    }

    std::shared_ptr<Attribute> AttributeIterator::ReadRawHeap(const AttributeId& attributeId)
    {
        BigInteger rawLength = ReadUnsignedLeb128();
        BigInteger rawOffset = ReadUnsignedLeb128();

        if (!rawLength.fits_sint_p())
        {
            throw HpkException("the length of the heap data is too large");
        }

        if (!rawOffset.fits_sint_p())
        {
            throw HpkException("the offset of the heap data is too large");
        }

        return std::make_shared<RawHeapAttribute>(
                attributeId,
                HeapCoordinates(
                        (size_t)rawOffset.get_ui(),
                        (size_t)rawLength.get_ui()));
    }

    BigInteger AttributeIterator::ReadUnsignedLeb128()
    {
        BigInteger result = 0;
        int shift = 0;

        while (true)
        {
            int b = context->HeapReader->ReadHeap(offset);
            offset++;

            result |= BigInteger(b & 0x7f) << shift;

            if (0 == (b & 0x80))
            {
                return result;
            }

            shift += 7;
        }

        return result;
    }

    void AttributeIterator::EnsureValidEncodingForInt(int encoding)
    {
        switch (encoding)
        {
            case ATTRIBUTE_ENCODING_INT_8_BIT:
            case ATTRIBUTE_ENCODING_INT_16_BIT:
            case ATTRIBUTE_ENCODING_INT_32_BIT:
            case ATTRIBUTE_ENCODING_INT_64_BIT:
                break;
            default:
                throw std::runtime_error("unknown encoding on a signed integer");
        }
    }
}