#include <string.h>

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

size_t strlcat(char * dst, const char * src, size_t maxlen)
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