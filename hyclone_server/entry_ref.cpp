#include <cstring>
#include <string>

#include "entry_ref.h"
#include "haiku_errors.h"
#include "process.h"
#include "server_servercalls.h"
#include "system.h"

#include <iostream>

intptr_t server_hserver_call_get_entry_ref(hserver_context& context, unsigned long long device, unsigned long long inode,
    const char* userPath, size_t userPathLength)
{
    auto& system = System::GetInstance();
    std::string path;

    {
        auto lock = system.Lock();

        if (system.GetEntryRef(EntryRef(device, inode), path) == -1)
        {
            return B_ENTRY_NOT_FOUND;
        }
    }

    size_t copyLen = path.size() + 1;

    if (copyLen > userPathLength)
    {
        return B_BUFFER_OVERFLOW;
    }

    auto lock = context.process->Lock();
    if (context.process->WriteMemory((void*)userPath, path.data(), copyLen) != copyLen)
    {
        return B_BAD_ADDRESS;
    }
    else
    {
        return copyLen;
    }
}

intptr_t server_hserver_call_register_entry_ref(hserver_context& context, unsigned long long device, unsigned long long inode,
    const char* userPath, size_t userPathLength)
{
    std::string path(userPathLength, '\0');

    {
        auto lock = context.process->Lock();
        if (context.process->ReadMemory((void*)userPath, path.data(), userPathLength) != userPathLength)
        {
            return B_BAD_ADDRESS;
        }
    }

    auto& system = System::GetInstance();
    path.resize(strlen(path.c_str()));
    while (path.size() > 1 && path.back() == '/')
        path.pop_back();
    path.shrink_to_fit();

    {
        auto lock = system.Lock();
        if (system.RegisterEntryRef(EntryRef(device, inode), std::move(path)) == -1)
        {
            return B_NO_MEMORY;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_register_entry_ref_child(hserver_context& context, unsigned long long pdevice, unsigned long long pinode,
    unsigned long long device, unsigned long long inode, const char* userPath, size_t userPathLength)
{
    std::string path(userPathLength, '\0');

    {
        auto lock = context.process->Lock();
        if (context.process->ReadMemory((void*)userPath, path.data(), userPathLength) != userPathLength)
        {
            return B_BAD_ADDRESS;
        }
    }

    auto& system = System::GetInstance();
    path.resize(strlen(path.c_str()));

    {
        auto lock = system.Lock();

        std::string parentPath;
        if (system.GetEntryRef(EntryRef(pdevice, pinode), parentPath) == -1)
        {
            return B_ENTRY_NOT_FOUND;
        }

        path = parentPath + "/" + path;
        path.shrink_to_fit();

        if (system.RegisterEntryRef(EntryRef(device, inode), std::move(path)) == -1)
        {
            return B_NO_MEMORY;
        }
    }

    return B_OK;
}