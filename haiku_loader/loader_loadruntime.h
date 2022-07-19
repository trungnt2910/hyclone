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

bool loader_load_runtime(runtime_loader_info* info);
void loader_unload_runtime(runtime_loader_info* info);

#endif // __LOADER_LOADRUNTIME_H__