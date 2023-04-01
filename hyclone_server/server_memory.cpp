#include <filesystem>
#include <vector>

#include "server_memory.h"
#include "server_prefix.h"
#include "servercalls.h"

static bool IsSubDir(const std::filesystem::path& parent, const std::filesystem::path& child)
{
    auto parentIt = parent.begin();
    auto childIt = child.begin();

    while (parentIt != parent.end() && childIt != child.end() && *parentIt == *childIt)
    {
        ++parentIt;
        ++childIt;
    }

    return parentIt == parent.end();
}

bool MemoryService::CreateSharedFile(const std::string& name, size_t size, std::string& hostPath)
{
    intptr_t handle = server_open_shared_file(name.c_str(), size, true);
    if (handle == -1)
    {
        return false;
    }

    server_close_file(handle);

    hostPath = (std::filesystem::path(gHaikuPrefix) / HYCLONE_SHM_NAME / name).string();
    return true;
}

bool MemoryService::CloneSharedFile(const std::string& name, const EntryRef& ref, std::string& hostPath)
{
    if (!_sharedFiles.contains(ref))
    {
        return false;
    }

    auto& file = _sharedFiles[ref];
    if (file.HasHandle())
    {
        return false;
    }

    intptr_t handle = server_clone_shared_file(name.c_str(), file.GetName().c_str(), true);
    if (handle == -1)
    {
        return false;
    }

    server_close_file(handle);

    hostPath = (std::filesystem::path(gHaikuPrefix) / HYCLONE_SHM_NAME / name).string();
    return true;
}

bool MemoryService::OpenSharedFile(int pid, intptr_t userHandle, bool writable, EntryRef& ref)
{
    intptr_t handle = server_acquire_process_file_handle(pid, userHandle, true);

    if (handle == -1 && writable)
    {
        return false;
    }
    else if (handle == -1)
    {
        handle = server_acquire_process_file_handle(pid, userHandle, false);
        if (handle == -1)
        {
            return false;
        }
    }
    else
    {
        writable = true;
    }

    if (!server_get_entry_ref(handle, ref))
    {
        server_close_file(handle);
        return false;
    }

    if (_sharedFiles.contains(ref))
    {
        auto& file = _sharedFiles[ref];
        file.IncrementRefCount();

        if (writable && !file.IsWritable())
        {
            if (file.HasHandle())
            {
                server_close_file(file.HasHandle());
            }
            file.SetHandle(handle);
            file.SetWritable();
        }
        else
        {
            server_close_file(handle);
        }

        return true;
    }
    else
    {
        _sharedFiles.emplace(ref, SharedFile(handle, writable));
        return true;
    }
}

bool MemoryService::OpenSharedFile(const std::string& name, bool writable, EntryRef& ref)
{
    intptr_t handle = server_open_file(name.c_str(), true);

    if (handle == -1 && writable)
    {
        return false;
    }
    else if (handle == -1)
    {
        handle = server_open_file(name.c_str(), false);
        if (handle == -1)
        {
            return false;
        }
    }
    else
    {
        writable = true;
    }

    if (!server_get_entry_ref(handle, ref))
    {
        server_close_file(handle);
        return false;
    }

    server_close_file(handle);

    if (_sharedFiles.contains(ref))
    {
        auto& file = _sharedFiles[ref];
        file.IncrementRefCount();

        if (writable && !file.IsWritable())
        {
            if (file.HasHandle())
            {
                server_close_file(file.GetHandle());
            }

            file.SetName(name);
            file.SetWritable();
        }

        return true;
    }
    else
    {
        _sharedFiles.emplace(ref, SharedFile(name, writable));
        return true;
    }
}

bool MemoryService::AcquireSharedFile(const EntryRef& ref)
{
    if (_sharedFiles.contains(ref))
    {
        _sharedFiles[ref].IncrementRefCount();
        return true;
    }
    return false;
}

bool MemoryService::GetSharedFilePath(const EntryRef& ref, std::string& path)
{
    if (!_sharedFiles.contains(ref))
    {
        return false;
    }

    auto& file = _sharedFiles[ref];
    if (file.HasHandle())
    {
        return false;
    }
    else
    {
        path = file.GetName();
    }

    return true;
}

void* MemoryService::AcquireMemory(const EntryRef& ref, size_t size, size_t offset, bool writable)
{
    if (!_sharedFiles.contains(ref))
    {
        return NULL;
    }

    auto& file = _sharedFiles[ref];
    if (writable && !file.IsWritable())
    {
        return NULL;
    }

    if (file.HasHandle())
    {
        return server_map_memory(file.GetHandle(), offset, size, writable);
    }
    else
    {
        intptr_t handle = server_open_file(file.GetName().c_str(), writable);
        if (handle == -1)
        {
            return NULL;
        }
        void* address = server_map_memory(handle, size, offset, writable);
        server_close_file(handle);
        return address;
    }
}

void MemoryService::ReleaseMemory(void* address, size_t size)
{
    server_unmap_memory(address, size);
}

void MemoryService::ReleaseSharedFile(const EntryRef& ref)
{
    if (_sharedFiles.contains(ref))
    {
        auto& file = _sharedFiles[ref];
        file.DecrementRefCount();

        if (file.GetRefCount() == 0)
        {
            if (file.HasHandle())
            {
                server_close_file(file.GetHandle());
            }
            else
            {
                auto sharedFilePath = std::filesystem::path(file.GetName());
                if (IsSubDir(std::filesystem::path(gHaikuPrefix) / HYCLONE_SHM_NAME, sharedFilePath))
                {
                    std::filesystem::remove(sharedFilePath);
                }
            }

            _sharedFiles.erase(ref);
        }
    }
}