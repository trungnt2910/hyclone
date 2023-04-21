#include <fstream>
#include <iostream>

#include "server_prefix.h"
#include "server_servercalls.h"
#include "server_usermap.h"
#include "system.h"

User::User(const std::string& passwdEntry)
{
    std::string token;
    std::stringstream ss(passwdEntry);

    std::getline(ss, token, ':');
    _name = token;

    std::getline(ss, token, ':');
    _passwordHash = token;

    std::getline(ss, token, ':');
    _uid = std::stoi(token);

    std::getline(ss, token, ':');
    _gid = std::stoi(token);

    std::getline(ss, token, ':');
    _info = token;

    std::getline(ss, token, ':');
    _homeDir = token;

    std::getline(ss, token, ':');
    _shell = token;
}

std::string User::ToString() const
{
    std::stringstream ss;
    ss << _name << ":"
        << _passwordHash << ":"
        << _uid << ":"
        << _gid << ":"
        << _info << ":"
        << _homeDir << ":"
        << _shell;
    return ss.str();
}

Group::Group(const std::string& groupEntry)
{
    std::string token;
    std::stringstream ss(groupEntry);

    std::getline(ss, token, ':');
    _name = token;

    std::getline(ss, token, ':');
    _passwordHash = token;

    std::getline(ss, token, ':');
    _gid = std::stoi(token);

    std::getline(ss, token, ':');
    std::stringstream ss2(token);
    while (std::getline(ss2, token, ','))
    {
        _users.push_back(token);
    }
}

std::string Group::ToString() const
{
    std::stringstream ss;
    ss << _name << ":"
        << _passwordHash << ":"
        << _gid << ":";
    for (auto& user : _users)
    {
        ss << user << ",";
    }
    return ss.str();
}

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

status_t UserMapService::GetHomeDir(int uid, std::filesystem::path& homeDir) const
{
    auto passwdPath = std::filesystem::path(gHaikuPrefix) / "boot/system/etc/passwd";
    std::ifstream passwdFile(passwdPath);

    std::string line;

    while (std::getline(passwdFile, line))
    {
        User user(line);
        if (user.GetUid() == uid)
        {
            homeDir = user.GetHomeDir();
            return B_OK;
        }
    }

    return B_BAD_VALUE;
}

bool server_setup_usermap()
{
    std::vector<HostUser> hostUsers;
    std::vector<HostGroup> hostGroups;
    std::unordered_map<std::string, HostUser*> hostUserMap;
    std::unordered_map<std::string, HostGroup*> hostGroupMap;

    if (!server_load_host_users(hostUsers, hostGroups))
    {
        std::cerr << "Failed to load host users and groups" << std::endl;
        return false;
    }

    assert(!hostUsers.empty());
    assert(!hostGroups.empty());

    intptr_t hycloneOwnerUid = hostUsers[0].uid;
    intptr_t hycloneOwnerGid = hostGroups[0].gid;

    std::cerr << "HyClone owner UID: " << hycloneOwnerUid << std::endl;
    std::cerr << "HyClone owner GID: " << hycloneOwnerGid << std::endl;

    auto& userMapService = System::GetInstance().GetUserMapService();

    userMapService.RegisterUid(hycloneOwnerUid, 0);
    userMapService.RegisterGid(hycloneOwnerGid, 0);

    std::sort(hostUsers.begin() + 1, hostUsers.end(), [](const auto& a, const auto& b) { return a.uid < b.uid; });
    std::sort(hostGroups.begin() + 1, hostGroups.end(), [](const auto& a, const auto& b) { return a.gid < b.gid; });

    for (size_t i = 1; i < hostUsers.size(); ++i)
    {
        userMapService.RegisterUid(hostUsers[i].uid, HYCLONE_MIN_HOST_UID + i - 1);
        hostUserMap[hostUsers[i].name] = &hostUsers[i];
    }

    for (size_t i = 1; i < hostGroups.size(); ++i)
    {
        userMapService.RegisterGid(hostGroups[i].gid, HYCLONE_MIN_HOST_GID + i - 1);
        hostGroupMap[hostGroups[i].name] = &hostGroups[i];
    }

    auto etcPath = std::filesystem::path(gHaikuPrefix) / "boot/system/etc";
    auto passwdPath = etcPath / "passwd";
    auto groupPath = etcPath / "group";

    std::vector<User> users;
    std::vector<Group> groups;

    {
        std::ifstream passwdFile(passwdPath);
        if (passwdFile.is_open())
        {
            std::string line;
            while (std::getline(passwdFile, line))
            {
                users.push_back(User(line));
            }
        }

        std::ifstream groupFile(groupPath);
        if (groupFile.is_open())
        {
            std::string line;
            while (std::getline(groupFile, line))
            {
                groups.push_back(Group(line));
            }
        }
    }

    std::vector<User> newUsers;
    std::vector<Group> newGroups;

    bool rootUserAdded = false;
    bool rootGroupAdded = false;

    for (auto& user: users)
    {
        if (user.GetUid() == 0)
        {
            rootUserAdded = true;
            user.SetName(hostUsers[0].name);
        }
        else if (user.GetUid() < HYCLONE_MIN_HOST_UID)
        {
            // Don't register this to the map.
            // When it encounters an unknown group, it will use the launcher's group.
        }
        else if (hostUserMap.find(user.GetName()) != hostUserMap.end())
        {
            auto hostUser = hostUserMap[user.GetName()];
            user.SetUid(userMapService.GetUid(hostUser->uid));
            user.SetGid(userMapService.GetGid(hostUser->gid));
            hostUserMap.erase(user.GetName());
        }
        else
        {
            continue;
        }

        newUsers.push_back(std::move(user));
    }

    for (auto& group: groups)
    {
        if (group.GetGid() == 0)
        {
            rootGroupAdded = true;
            group.SetName(hostGroups[0].name);
        }
        else if (group.GetGid() < HYCLONE_MIN_HOST_GID)
        {
            // Don't register this to the map.
            // When it encounters an unknown group, it will use the launcher's group.
        }
        else if (hostGroupMap.find(group.GetName()) != hostGroupMap.end())
        {
            auto hostGroup = hostGroupMap[group.GetName()];
            group.SetGid(userMapService.GetGid(hostGroup->gid));
            group.SetUsers(hostGroup->users);
            hostGroupMap.erase(group.GetName());
        }
        else
        {
            continue;
        }

        newGroups.push_back(std::move(group));
    }

    for (auto& [name, hostUser]: hostUserMap)
    {
        User user;
        user.SetName(name);
        user.SetUid(userMapService.GetUid(hostUser->uid));
        user.SetGid(userMapService.GetGid(hostUser->gid));
        newUsers.push_back(std::move(user));
    }

    for (auto& [name, hostGroup]: hostGroupMap)
    {
        Group group;
        group.SetName(name);
        group.SetGid(userMapService.GetGid(hostGroup->gid));
        group.SetUsers(hostGroup->users);
        newGroups.push_back(std::move(group));
    }

    if (!rootUserAdded)
    {
        User user;
        user.SetName(hostUsers[0].name);
        user.SetUid(0);
        user.SetGid(0);
        newUsers.push_back(std::move(user));
    }

    if (!rootGroupAdded)
    {
        Group group;
        group.SetName(hostGroups[0].name);
        group.SetGid(0);
        group.SetUsers({hostGroups[0].name});
        newGroups.push_back(std::move(group));
    }

    {
        std::ofstream passwdFile(passwdPath);
        if (!passwdFile.is_open())
        {
            std::cerr << "Failed to open " << passwdPath << std::endl;
            return false;
        }

        for (auto& user: newUsers)
        {
            passwdFile << user.ToString() << "\n";
        }

        std::ofstream groupFile(groupPath);

        if (!groupFile.is_open())
        {
            std::cerr << "Failed to open " << groupPath << std::endl;
            return false;
        }

        for (auto& group: newGroups)
        {
            groupFile << group.ToString() << "\n";
        }
    }

    return true;
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