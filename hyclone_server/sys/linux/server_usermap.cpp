#include <fstream>
#include <unistd.h>

#include "server_usermap.h"
#include "system.h"

bool server_load_host_users(std::vector<HostUser>& users, std::vector<HostGroup>& groups)
{
    // Get the host user and group for the current process
    intptr_t hostUid = getuid();
    intptr_t hostGid = getgid();

    // Open /etc/passwd and read users
    std::ifstream passwdFile("/etc/passwd");

    if (!passwdFile.is_open())
    {
        return false;
    }

    std::string line;

    while (std::getline(passwdFile, line))
    {
        User user(line);
        if (user.GetUid() < 1000)
        {
            user.SetName("host-" + user.GetName());
        }
        users.push_back({user.GetName(), user.GetUid(), user.GetGid()});
    }

    // Open /etc/group and read groups
    std::ifstream groupFile("/etc/group");

    if (!groupFile.is_open())
    {
        return false;
    }

    while (std::getline(groupFile, line))
    {
        Group group(line);
        if (group.GetGid() < 1000)
        {
            group.SetName("host-" + group.GetName());
        }
        groups.push_back({group.GetName(), group.GetGid(), group.GetUsers()});
    }

    for (auto& user: users)
    {
        if (user.uid == hostUid)
        {
            std::swap(user, users[0]);
            break;
        }
    }

    for (auto& group: groups)
    {
        if (group.gid == hostGid)
        {
            std::swap(group, groups[0]);
            break;
        }
    }

    return true;
}