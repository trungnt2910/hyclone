#ifndef __HAIKU_RUNTIME_LOADER__
#define __HAIKU_RUNTIME_LOADER__

#include "BeDefs.h"

struct rld_export 
{
    // runtime loader API export
    image_id (*load_add_on)(char const *path, uint32 flags);
    status_t (*unload_add_on)(image_id imageID);
    image_id (*load_library)(char const *path, uint32 flags, void* caller,
        void **_handle);
    status_t (*unload_library)(void* handle);
    status_t (*get_image_symbol)(image_id imageID, char const *symbolName,
        int32 symbolType, bool recursive, image_id *_inImage, void **_location);
    status_t (*get_library_symbol)(void* handle, void* caller,
        const char* symbolName, void **_location);
    status_t (*get_nth_image_symbol)(image_id imageID, int32 num,
        char *symbolName, int32 *nameLength, int32 *symbolType,
        void **_location);
    status_t (*get_nearest_symbol_at_address)(void* address,
        image_id* _imageID,	char** _imagePath, char** _imageName,
        char** _symbolName, int32* _type, void** _location, bool* _exactMatch);
    status_t (*test_executable)(const char *path, char *interpreter);
    status_t (*get_executable_architecture)(const char *path,
        const char** _architecture);
    status_t (*get_next_image_dependency)(image_id id, uint32 *cookie,
        const char **_name);
    void* (*get_tls_address)(unsigned dso, addr_t offset);
    void (*destroy_thread_tls)();

    status_t (*reinit_after_fork)();

    void (*call_atexit_hooks_for_range)(addr_t start, addr_t size);

    void (*call_termination_hooks)();

    const struct user_space_program_args *program_args;
    const void* commpage_address;
    int abi_version;
    int api_version;
};

#endif