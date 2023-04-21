#ifndef __SERVER_USERMAP_H__
#define __SERVER_USERMAP_H__

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "BeDefs.h"

struct HostUser
{
    std::string name;
    intptr_t uid;
    intptr_t gid;
};

struct HostGroup
{
    std::string name;
    intptr_t gid;
    std::vector<std::string> users;
};

bool server_setup_usermap();

// The first elements of this vector should correspond to the user and group
// who launched hyclone_server (the emulated root).
bool server_load_host_users(std::vector<HostUser>& users,
    std::vector<HostGroup>& groups);

class User
{
private:
    int _uid;
    int _gid;
    std::string _name;
    std::string _passwordHash = "x";
    std::string _info;
    std::string _homeDir = "/boot/home";
    std::string _shell = "/bin/bash";
public:
    User() = default;
    User(const std::string& passwdEntry);
    User(const User& other) = default;
    User(User&& other) = default;

    std::string ToString() const;

    int GetUid() const { return _uid; }
    int GetGid() const { return _gid; }
    const std::string& GetName() const { return _name; }
    const std::string& GetPasswordHash() const { return _passwordHash; }
    const std::string& GetInfo() const { return _info; }
    const std::string& GetHomeDir() const { return _homeDir; }
    const std::string& GetShell() const { return _shell; }

    void SetUid(int uid) { _uid = uid; }
    void SetGid(int gid) { _gid = gid; }
    void SetName(const std::string& name) { _name = name; }
    void SetPasswordHash(const std::string& passwordHash) { _passwordHash = passwordHash; }
    void SetInfo(const std::string& info) { _info = info; }
    void SetHomeDir(const std::string& homeDir) { _homeDir = homeDir; }
    void SetShell(const std::string& shell) { _shell = shell; }
};

class Group
{
private:
    std::string _name;
    std::string _passwordHash = "x";
    std::vector<std::string> _users;
    int _gid;
public:
    Group() = default;
    Group(const std::string& groupEntry);
    Group(const Group& other) = default;
    Group(Group&& other) = default;

    std::string ToString() const;

    const std::string& GetName() const { return _name; }
    const std::string& GetPasswordHash() const { return _passwordHash; }
    const std::vector<std::string>& GetUsers() const { return _users; }
    int GetGid() const { return _gid; }

    std::vector<std::string>& GetUsers() { return _users; }

    void SetName(const std::string& name) { _name = name; }
    void SetPasswordHash(const std::string& passwordHash) { _passwordHash = passwordHash; }
    void SetUsers(const std::vector<std::string>& users) { _users = users; }
    void SetGid(int gid) { _gid = gid; }
};

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

    status_t GetHomeDir(int uid,  std::filesystem::path& homeDir) const;

    std::unique_lock<std::mutex> Lock() { return std::unique_lock<std::mutex>(_lock); }
};

#define HYCLONE_MIN_HOST_UID 2000
#define HYCLONE_MIN_HOST_GID 2000

#endif // __SERVER_USERMAP_H__