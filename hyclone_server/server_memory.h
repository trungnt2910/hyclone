#ifndef __SERVER_MEMORY_H__
#define __SERVER_MEMORY_H__

#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include "entry_ref.h"

// Initializes the memory subsystem
bool server_setup_memory();
// Opens a shared file that can be mapped to any process's address space
// The file should be accessible by monika at gHaikuPrefix / HYCLONE_SHM_NAME / name
intptr_t server_open_shared_file(const char* name, size_t size, bool writable);
// Clones the file located at path into a shared file
intptr_t server_clone_shared_file(const char* name, const char* path, bool writable);
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
    class SharedFile
    {
    private:
        std::variant<std::string, intptr_t> _handleOrName = -1;
        bool _writable = false;
        size_t _refCount = 1;
    public:
        SharedFile(const std::string& name, bool writable) :
            _handleOrName(name), _writable(writable) { }
        SharedFile(intptr_t handle, bool writable) :
            _handleOrName(handle), _writable(writable) { }
        SharedFile() = default;
        bool IsWritable() const { return _writable; }
        void SetWritable(bool writable = true) { _writable = writable; }
        bool HasHandle() const { return std::holds_alternative<intptr_t>(_handleOrName); }
        intptr_t GetHandle() const { return std::get<intptr_t>(_handleOrName); }
        void SetHandle(intptr_t handle) { _handleOrName = handle; }
        const std::string& GetName() const { return std::get<std::string>(_handleOrName); }
        void SetName(const std::string& name) { _handleOrName = name; }
        size_t GetRefCount() const { return _refCount; }
        void IncrementRefCount() { ++_refCount; }
        void DecrementRefCount() { --_refCount; }
    };

    std::unordered_map<EntryRef, SharedFile> _sharedFiles;
    std::mutex _lock;
public:
    MemoryService() = default;
    ~MemoryService() = default;

    bool CreateSharedFile(const std::string& name, size_t size, std::string& hostPath);
    bool CloneSharedFile(const std::string& name, const EntryRef& ref, std::string& hostPath);
    bool OpenSharedFile(int pid, intptr_t userHandle, bool writable, EntryRef& ref);
    bool OpenSharedFile(const std::string& name, bool writable, EntryRef& ref);
    bool AcquireSharedFile(const EntryRef& ref);

    void* AcquireMemory(const EntryRef& ref, size_t size, size_t offset, bool writable);
    void ReleaseMemory(void* address, size_t size);

    void ReleaseSharedFile(const EntryRef& ref);

    std::unique_lock<std::mutex> Lock() { return std::unique_lock(_lock); }
};

#endif // __SERVER_MEMORY_H__