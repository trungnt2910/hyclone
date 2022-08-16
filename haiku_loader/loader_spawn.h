#ifndef __LOADER_SPAWN_H__
#define __LOADER_SPAWN_H__

#include <cstddef>

int loader_spawn(const char* path, const char* const* flatArgs,
    size_t flatArgsSize, int argc, int envc,
    int priority, int flags, int errorPort, int errorToken);

#endif // __LOADER_SPAWN_H__