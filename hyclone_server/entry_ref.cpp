#include <cstring>
#include <string>

#include "entry_ref.h"
#include "haiku_errors.h"
#include "process.h"
#include "server_servercalls.h"
#include "system.h"

intptr_t server_hserver_call_get_entry_ref(hserver_context& context, unsigned long long device, unsigned long long inode,
    void* userNameAndSize, void* userPathAndSize, bool traverseLinks)
{
    auto& vfsService = System::GetInstance().GetVfsService();
    std::string pathStr;
    std::filesystem::path path;
    std::string name;

    {
        auto lock = context.process->Lock();
        std::pair<const char*, size_t> nameAndSize;
        if (context.process->ReadMemory(userNameAndSize, &nameAndSize, sizeof(nameAndSize)) != sizeof(nameAndSize))
        {
            return B_BAD_ADDRESS;
        }

        if (nameAndSize.second > 0)
        {
            name.resize(nameAndSize.second);
            if (context.process->ReadMemory((void*)nameAndSize.first, name.data(), nameAndSize.second) != nameAndSize.second)
            {
                return B_BAD_ADDRESS;
            }
        }
    }

    {
        auto lock = vfsService.Lock();

        if (!vfsService.GetEntryRef(EntryRef(device, inode), pathStr))
        {
            return B_ENTRY_NOT_FOUND;
        }

        if (!name.empty() && name != ".")
        {
            path = pathStr;
            path /= name;

            if (traverseLinks)
            {
                status_t status = vfsService.RealPath(path);
                if (status != B_OK)
                {
                    return status;
                }
            }

            path = path.lexically_normal();
            if (path.filename().empty())
            {
                path = path.parent_path();
            }

            pathStr = path.string();
        }
    }

    size_t copyLen = pathStr.size() + 1;

    {
        auto lock = context.process->Lock();
        std::pair<char*, size_t> pathAndSize;
        if (context.process->ReadMemory(userPathAndSize, &pathAndSize, sizeof(pathAndSize)) != sizeof(pathAndSize))
        {
            return B_BAD_ADDRESS;
        }

        if (copyLen > pathAndSize.second)
        {
            return B_BUFFER_OVERFLOW;
        }

        if (context.process->WriteMemory(pathAndSize.first, pathStr.data(), copyLen) != copyLen)
        {
            return B_BAD_ADDRESS;
        }
    }

    return copyLen;
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