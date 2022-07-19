#ifndef __LOADER_EXEC_H__
#define __LOADER_EXEC_H__

#include <cstddef>

int loader_exec(const char* path, const char* const* flatArgs, size_t flatArgsSize, int argc, int envc, int umask);

#endif // __LOADER_EXEC_H__