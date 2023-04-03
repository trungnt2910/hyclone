#include <unistd.h>

#include "server_usermap.h"
#include "system.h"

bool server_setup_usermap()
{
    auto& system = System::GetInstance();
    auto lock = system.Lock();

    auto& mapService = system.GetUserMapService();

    int uid = getuid();
    int gid = getgid();

    if (uid == -1 || gid == -1)
    {
        return false;
    }

    mapService.RegisterUid(uid, 0);
    mapService.RegisterGid(gid, 0);

    return true;
}