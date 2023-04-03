#ifndef __SERVER_USERMAP_H__
#define __SERVER_USERMAP_H__

#include <cstdint>
#include <mutex>
#include <unordered_map>

bool server_setup_usermap();

class UserMapService
{
private:
    std::mutex _lock;
    std::unordered_map<intptr_t, int> _uidMap;
    std::unordered_map<intptr_t, int> _gidMap;
    std::unordered_map<int, intptr_t> _uidReverseMap;
    std::unordered_map<int, intptr_t> _gidReverseMap;
public:
    UserMapService() = default;
    ~UserMapService() = default;

    void RegisterUid(intptr_t hostUid, int uid);
    void RegisterGid(intptr_t hostGid, int gid);

    int GetUid(intptr_t hostUid) const;
    int GetGid(intptr_t hostGid) const;

    intptr_t GetHostUid(int uid) const;
    intptr_t GetHostGid(int gid) const;

    std::unique_lock<std::mutex> Lock() { return std::unique_lock<std::mutex>(_lock); }
};

#endif // __SERVER_USERMAP_H__