#include <cstring>
#include <string>

#include "entry_ref.h"
#include "haiku_errors.h"
#include "process.h"
#include "server_servercalls.h"
#include "system.h"

intptr_t server_hserver_call_get_entry_ref(hserver_context& context, unsigned long long device, unsigned long long inode,
    char* userPath, size_t userPathLength)
{
    auto& vfsService = System::GetInstance().GetVfsService();
    std::string pathStr;
    std::filesystem::path path;

    {
        auto lock = vfsService.Lock();

        if (!vfsService.GetEntryRef(EntryRef(device, inode), pathStr))
        {
            return B_ENTRY_NOT_FOUND;
        }

        path = pathStr;
        status_t status = vfsService.GetPath(path, false);

        if (status != B_OK)
        {
            return status;
        }

        pathStr = path.string();
    }

    size_t copyLen = pathStr.size() + 1;

    if (copyLen > userPathLength)
    {
        return B_BUFFER_OVERFLOW;
    }

    auto lock = context.process->Lock();
    if (context.process->WriteMemory(userPath, pathStr.data(), copyLen) != copyLen)
    {
        return B_BAD_ADDRESS;
    }
    else
    {
        return copyLen;
    }
}

intptr_t server_hserver_call_register_entry_ref(hserver_context& context, unsigned long long device, unsigned long long inode,
    int fd)
{
    {
        auto lock = context.process->Lock();
        if (!context.process->IsValidFd(fd))
        {
            return HAIKU_POSIX_EBADF;
        }

        const auto& path = context.process->GetFd(fd);

        auto& vfsService = System::GetInstance().GetVfsService();
        auto vfsLock = vfsService.Lock();
        vfsService.RegisterEntryRef(EntryRef(device, inode), path.string());
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

    path.resize(strlen(path.c_str()));

    {
        auto& vfsService = System::GetInstance().GetVfsService();
        auto lock = vfsService.Lock();

        std::string parentPath;
        if (!vfsService.GetEntryRef(EntryRef(pdevice, pinode), parentPath))
        {
            return B_ENTRY_NOT_FOUND;
        }

        path = parentPath + "/" + path;
        path.shrink_to_fit();

        vfsService.RegisterEntryRef(EntryRef(device, inode), std::move(path));
    }

    return B_OK;
}