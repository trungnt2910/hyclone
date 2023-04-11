#include <string.h>

#include "BeDefs.h"
#include "stringutils.h"

size_t strlcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;
    /* Copy as many bytes as will fit */
    if (n != 0)
    {
        while (--n != 0)
        {
            if ((*d++ = *s++) == '\0')
                break;
        }
    }
    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0)
    {
        if (siz != 0)
            *d = '\0'; /* NUL-terminate dst */
        while (*s++)
            ;
    }
    return (s - src - 1); /* count does not include NUL */
}

size_t strlcat(char *dst, const char *src, size_t maxlen)
{
    const size_t srclen = strlen(src);
    const size_t dstlen = strnlen(dst, maxlen);
    if (dstlen == maxlen)
        return maxlen + srclen;
    if (srclen < maxlen - dstlen)
    {
        memcpy(dst + dstlen, src, srclen + 1);
    }
    else
    {
        memcpy(dst + dstlen, src, maxlen - dstlen - 1);
        dst[maxlen - 1] = '\0';
    }
    return dstlen + srclen;
}

/* From Bit twiddling hacks:
    http://graphics.stanford.edu/~seander/bithacks.html */
#define LACKS_ZERO_BYTE(value) \
    (((value - 0x01010101) & ~value & 0x80808080) == 0)

char * strncpy(char *dest, const char *src, size_t count)
{
    char *tmp = dest;

    // Align destination buffer for four byte writes.
    while (((addr_t)dest & 3) != 0 && count != 0)
    {
        count--;
        if ((*dest++ = *src++) == '\0')
        {
            memset(dest, '\0', count);
            return tmp;
        }
    }

    if (count == 0)
        return tmp;

    if (((addr_t)src & 3) == 0)
    {
        // If the source and destination are aligned, copy a word
        // word at a time
        uint32 *alignedSrc = (uint32 *)src;
        uint32 *alignedDest = (uint32 *)dest;
        size_t alignedCount = count / 4;
        count -= alignedCount * 4;

        for (; alignedCount != 0 && LACKS_ZERO_BYTE(*alignedSrc);
             alignedCount--)
            *alignedDest++ = *alignedSrc++;

        count += alignedCount * 4;
        src = (char *)alignedSrc;
        dest = (char *)alignedDest;
    }

    // Deal with the remainder.
    while (count-- != 0)
    {
        if ((*dest++ = *src++) == '\0')
        {
            memset(dest, '\0', count);
            return tmp;
        }
    }

    return tmp;
}