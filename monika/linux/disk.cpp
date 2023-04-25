#include <cstddef>
#include <cstring>
#include <utility>

#include "BeDefs.h"
#include "extended_commpage.h"
#include "haiku_errors.h"

extern "C"
{

extern void _moni_debug_output(const char* userString);

partition_id _moni_get_next_disk_device_id(int32* cookie, size_t* neededSize)
{
    // HyClone currently doesn't manage any disk device anyway.
    _moni_debug_output("stub: _kern_get_next_disk_device_id");
    return B_BAD_VALUE;
}

status_t _moni_start_watching_disks(uint32 eventMask, port_id port, int32 token)
{
    _moni_debug_output("stub: _kern_start_watching_disks");
    return HAIKU_POSIX_ENOSYS;
}

status_t _moni_start_watching(haiku_dev_t device, haiku_ino_t node, uint32 flags,
    port_id port, uint32 token)
{
    return GET_SERVERCALLS()->start_watching(device, node, flags, port, token);
}
status_t _moni_stop_watching(haiku_dev_t device, haiku_ino_t node, port_id port,
    uint32 token)
{
    return GET_SERVERCALLS()->stop_watching(device, node, port, token);
}

status_t _moni_stop_notifying(port_id port, uint32 token)
{
    return GET_SERVERCALLS()->stop_notifying(port, token);
}

haiku_dev_t _moni_next_device(int32* _cookie)
{
    return GET_SERVERCALLS()->next_device(_cookie);
}

haiku_dev_t _moni_mount(const char* path, const char* device,
    const char* fs_name, uint32 flags, const char* args,
    size_t argsLength)
{
    if (path == NULL)
    {
        return B_BAD_ADDRESS;
    }

    if (*path == '\0')
    {
        return B_ENTRY_NOT_FOUND;
    }

    std::pair<const char*, size_t> pathAndSize(path, strlen(path));
    std::pair<const char*, size_t> deviceAndSize(device, device ? strlen(device) : 0);
    std::pair<const char*, size_t> fsNameAndSize(fs_name, fs_name ? strlen(fs_name) : 0);

    return GET_SERVERCALLS()->mount(&pathAndSize, &deviceAndSize, &fsNameAndSize, flags, args, argsLength);
}

status_t _moni_unmount(const char* path, uint32 flags)
{
    if (path == NULL)
    {
        return B_BAD_ADDRESS;
    }

    return GET_SERVERCALLS()->unmount(path, strlen(path), flags);
}

}