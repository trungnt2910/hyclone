#ifndef __STRINGUTILS_H__
#define __STRINGUTILS_H__

#include <cstddef>

extern "C"
{

size_t stringutils_strlcpy(char *dst, const char *src, size_t siz);
size_t stringutils_strlcat(char * dst, const char * src, size_t maxlen);

}

// Avoid link-time clashes with symbols in libroot.
#define strlcpy stringutils_strlcpy
#define strlcat stringutils_strlcat

#define IS_NULL_OR_EMPTY(str) ((str) == NULL || (str)[0] == '\0')

#endif // __STRINGUTILS_H__