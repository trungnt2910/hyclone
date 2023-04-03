#include "server_servercalls.h"
#include "system.h"
#include "server_usermap.h"

void UserMapService::RegisterUid(intptr_t hostUid, int uid)
{
    _uidMap[hostUid] = uid;
    _uidReverseMap[uid] = hostUid;
}

void UserMapService::RegisterGid(intptr_t hostGid, int gid)
{
    _gidMap[hostGid] = gid;
    _gidReverseMap[gid] = hostGid;
}

int UserMapService::GetUid(intptr_t hostUid) const
{
    auto it = _uidMap.find(hostUid);
    if (it == _uidMap.end())
    {
        return hostUid;
    }
    return it->second;
}

int UserMapService::GetGid(intptr_t hostGid) const
{
    auto it = _gidMap.find(hostGid);
    if (it == _gidMap.end())
    {
        return hostGid;
    }
    return it->second;
}

intptr_t UserMapService::GetHostUid(int uid) const
{
    auto it = _uidReverseMap.find(uid);
    if (it == _uidReverseMap.end())
    {
        return -1;
    }
    return it->second;
}

intptr_t UserMapService::GetHostGid(int gid) const
{
    auto it = _gidReverseMap.find(gid);
    if (it == _gidReverseMap.end())
    {
        return -1;
    }
    return it->second;
}

intptr_t server_hserver_call_uid_for(hserver_context& context, intptr_t hostUid)
{
    auto& userMapService = System::GetInstance().GetUserMapService();
    auto lock = userMapService.Lock();
    return userMapService.GetUid(hostUid);
}

intptr_t server_hserver_call_gid_for(hserver_context& context, intptr_t hostGid)
{
    auto& userMapService = System::GetInstance().GetUserMapService();
    auto lock = userMapService.Lock();
    return userMapService.GetGid(hostGid);
}

intptr_t server_hserver_call_hostuid_for(hserver_context& context, int uid)
{
    auto& userMapService = System::GetInstance().GetUserMapService();
    auto lock = userMapService.Lock();
    return userMapService.GetHostUid(uid);
}

intptr_t server_hserver_call_hostgid_for(hserver_context& context, int gid)
{
    auto& userMapService = System::GetInstance().GetUserMapService();
    auto lock = userMapService.Lock();
    return userMapService.GetHostGid(gid);
}