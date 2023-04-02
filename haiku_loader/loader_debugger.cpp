#include <cstdarg>
#include <cstdio>

#include "loader_debugger.h"

int loader_dprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vfprintf(stderr, format, args);
    va_end(args);

    return result;
}