#ifndef __SERVER_MEMORY_H__
#define __SERVER_MEMORY_H__

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>

#include "entry_ref.h"

// Initializes the memory subsystem
bool server_setup_memory();
// Opens a shared file that can be mapped to any process's address space
// The file should be accessible by monika at gHaikuPrefix / HYCLONE_SHM_NAME / name
intptr_t server_open_shared_file(const char* name, size_t size, bool writable);
// Opens an existing file
intptr_t server_open_file(const char* name, bool writable);
// Resizes a shared file
bool server_resize_shared_file(intptr_t handle, size_t size);
// Closes a shared file
void server_close_file(intptr_t handle);
// Acquires a file handle for this process based on a file handle from another process
intptr_t server_acquire_process_file_handle(int pid, intptr_t handle, bool writable);
// Get EntryRef from handle
bool server_get_entry_ref(intptr_t handle, EntryRef& ref);
// Maps a file into this process's address space
void* server_map_memory(intptr_t handle, size_t size, size_t offset, bool writable);
// Resizes a mapping
void* server_resize_memory(void* address, size_t size, size_t newSize);
// Unmaps a file from this process's address space
void server_unmap_memory(void* address, size_t size);
// Gets the server page size
size_t server_get_page_size();

class MemoryService
{
private:
    struct SharedFile
    {
        union
        {
            std::string name;
            intptr_t handle = -1;
        };
        bool hasHandle = true;
        bool writable = false;
        size_t refCount = 0;

        SharedFile(const std::string& name, bool writable) :
            name(name),
            hasHandle(false),
            writable(writable),
            refCount(1)
        {
        }

        SharedFile(intptr_t handle, bool writable) :
            handle(handle),
            hasHandle(true),
            writable(writable),
            refCount(1)
        {
        }

        SharedFile()
        {
        }

        SharedFile(SharedFile&& other)
        {
            if (other.hasHandle)
            {
                handle = other.handle;
                hasHandle = true;
            }
            else
            {
                new (&name) std::string(std::move(other.name));
                hasHandle = false;
            }
            writable = other.writable;
            refCount = other.refCount;
        }

        ~SharedFile()
        {
            if (hasHandle)
            {
                server_close_file(handle);
            }
            else
            {
                name.~basic_string();
            }
        }
    };

    std::unordered_map<EntryRef, SharedFile> _sharedFiles;
    std::unordered_map<EntryRef, std::unordered_map<size_t, std::pair<void*, bool>>> _mappedFiles;
public:
    MemoryService() = default;
    ~MemoryService() = default;

    bool OpenSharedFile(int pid, intptr_t userHandle, bool writable, EntryRef& ref);
    bool OpenSharedFile(const std::string& name, bool writable, EntryRef& ref);

    // Access exactly one page of memory
    void* AccessMemory(const EntryRef& ref, size_t offset, bool writable);

    void ReleaseSharedFile(const EntryRef& ref);
};

#endif // __SERVER_MEMORY_H__