#include <vector>

#include "server_memory.h"


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
        ++file.refCount;

        if (writable && !file.writable)
        {
            if (!file.hasHandle)
            {
                file.hasHandle = true;
                file.name.~basic_string();
                file.handle = handle;
            }
            else
            {
                server_close_file(file.handle);
                file.handle = handle;
            }

            file.writable = true;

            // Unmap all read-only regions in favor of writable ones
            if (_mappedFiles.contains(ref))
            {
                std::vector<size_t> toRemove;
                auto& map = _mappedFiles[ref];

                for (auto& [offset, pair] : map)
                {
                    if (!pair.second)
                    {
                        server_unmap_memory(pair.first, server_get_page_size());
                        pair.first = server_map_memory(handle, offset, server_get_page_size(), true);
                        pair.second = true;

                        if (pair.first == nullptr)
                        {
                            toRemove.push_back(offset);
                        }
                    }
                }

                for (auto offset : toRemove)
                {
                    map.erase(offset);
                }
            }
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
        ++file.refCount;

        if (writable && !file.writable)
        {
            if (file.hasHandle)
            {
                server_close_file(file.handle);
                file.hasHandle = false;
                new (&file.name) std::string(name);
            }
            else
            {
                file.name = name;
            }

            file.writable = true;

            // Unmap all read-only regions in favor of writable ones
            if (_mappedFiles.contains(ref))
            {
                std::vector<size_t> toRemove;
                auto& map = _mappedFiles[ref];

                for (auto& [offset, pair] : map)
                {
                    if (!pair.second)
                    {
                        server_unmap_memory(pair.first, server_get_page_size());
                        pair.first = server_map_memory(handle, offset, server_get_page_size(), true);
                        pair.second = true;

                        if (pair.first == nullptr)
                        {
                            toRemove.push_back(offset);
                        }
                    }
                }

                for (auto offset : toRemove)
                {
                    map.erase(offset);
                }
            }
        }

        return true;
    }
    else
    {
        _sharedFiles.emplace(ref, SharedFile(name, writable));
        return true;
    }
}

void* MemoryService::AccessMemory(const EntryRef& ref, size_t offset, bool writable)
{
    if (!_sharedFiles.contains(ref))
    {
        return NULL;
    }

    auto& file = _sharedFiles[ref];

    if (writable && !file.writable)
    {
        return NULL;
    }

    intptr_t handle;
    if (file.hasHandle)
    {
        handle = file.handle;
    }
    else
    {
        handle = server_open_file(file.name.c_str(), file.writable);
        if (handle == -1)
        {
            return NULL;
        }
    }

    size_t roundedOffset = offset & ~(server_get_page_size() - 1);

    if (_mappedFiles.contains(ref))
    {
        auto& map = _mappedFiles[ref];

        if (map.contains(roundedOffset))
        {
            auto& pair = map[roundedOffset];

            if (writable && !pair.second)
            {
                server_unmap_memory(pair.first, server_get_page_size());
                pair.first = server_map_memory(file.handle, roundedOffset, server_get_page_size(), true);
                pair.second = true;

                if (pair.first == NULL)
                {
                    map.erase(roundedOffset);
                    if (!file.hasHandle)
                    {
                        server_close_file(handle);
                    }
                    return NULL;
                }
            }

            if (!file.hasHandle)
            {
                server_close_file(handle);
            }
            return pair.first;
        }
    }

    void* ptr = server_map_memory(file.handle, roundedOffset, server_get_page_size(), writable);

    if (!file.hasHandle)
    {
        server_close_file(handle);
    }

    if (ptr == NULL)
    {
        return NULL;
    }

    if (!_mappedFiles.contains(ref))
    {
        _mappedFiles.emplace(ref, std::unordered_map<size_t, std::pair<void*, bool>>());
    }

    _mappedFiles[ref].emplace(roundedOffset, std::make_pair(ptr, writable));
    return ((uint8_t*)ptr) + (offset - roundedOffset);
}

void MemoryService::ReleaseSharedFile(const EntryRef& ref)
{
    if (_sharedFiles.contains(ref))
    {
        auto& file = _sharedFiles[ref];
        --file.refCount;

        if (file.refCount == 0)
        {
            // Let the desctructor take care of SharedFile
            // and its paths/handles.
            _sharedFiles.erase(ref);

            if (_mappedFiles.contains(ref))
            {
                auto& map = _mappedFiles[ref];

                for (auto& [offset, pair] : map)
                {
                    server_unmap_memory(pair.first, server_get_page_size());
                }

                _mappedFiles.erase(ref);
            }
        }
    }
}