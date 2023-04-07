#ifndef __LOADER_LOADRUNTIME_H__
#define __LOADER_LOADRUNTIME_H__

#include "runtime_loader.h"

typedef int (*runtime_loader_main_func)(void* args, void* commpage);

struct runtime_loader_info
{
    void*                       handle;
    runtime_loader_main_func    entry_point;
    rld_export**                gRuntimeLoaderPtr;
};

extern runtime_loader_info gRuntimeLoaderInfo;

bool loader_load_runtime();
void loader_unload_runtime();
void* loader_runtime_symbol(const char* name);

#endif // __LOADER_LOADRUNTIME_H__