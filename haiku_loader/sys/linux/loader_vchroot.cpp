#include <cassert>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <vector>
#include <unistd.h>
#include "loader_vchroot.h"

std::string gHaikuPrefix = "";

static const char* kHostMountPoint = "/SystemRoot/";
static std::vector<std::string> sHaikuPrefixParts;

static std::vector<std::string> GetParts(const std::string& path);
static std::string HaikuPathFromFdAndPath(int fd, const char* path);
static std::string HostPathFromFd(int fd);

bool loader_init_vchroot(const std::filesystem::path& hprefix)
{
    gHaikuPrefix = std::filesystem::canonical(hprefix).string();
    if (gHaikuPrefix.empty())
    {
        return false;
    }
    if (gHaikuPrefix.back() == '/')
    {
        gHaikuPrefix.pop_back();
    }
    sHaikuPrefixParts = GetParts(gHaikuPrefix);
    return true;
}

size_t loader_vchroot_expand(const char* path, char* hostPath, size_t size)
{
    std::string haikuRealPath;
    if (path[0] == '/')
    {
        haikuRealPath = path;
    }
    else
    {
        std::string cwd = std::filesystem::current_path().string();
        size_t cwdLen = loader_vchroot_unexpand(cwd.c_str(), nullptr, 0);
        std::string haikuCwd(cwdLen, '\0');
        loader_vchroot_unexpand(cwd.c_str(), haikuCwd.data(), cwdLen);
        haikuRealPath = (std::filesystem::path(haikuCwd) / std::filesystem::path(path)).string();
    }

    std::string hostRealPath = "/";

    std::vector<std::string> haikuRealPathParts = GetParts(haikuRealPath);
    if (haikuRealPathParts.size() > 0 && haikuRealPathParts[0] == "SystemRoot")
    {
        for (size_t i = 1; i < haikuRealPathParts.size(); ++i)
        {
            hostRealPath += haikuRealPathParts[i];
            if (i != haikuRealPathParts.size() - 1)
            {
                hostRealPath += "/";
            }
        }
    }
    // Reflect the whole Linux devfs to Haiku.
    else if (haikuRealPathParts.size() > 0 && haikuRealPathParts[0] == "dev")
    {
        hostRealPath = haikuRealPath;
    }
    else
    {
        hostRealPath = gHaikuPrefix + "/";
        for (size_t i = 0; i < haikuRealPathParts.size(); ++i)
        {
            hostRealPath += haikuRealPathParts[i];
            if (i != haikuRealPathParts.size() - 1)
            {
                hostRealPath += "/";
            }
        }
    }

    if (hostPath != NULL)
    {
        strncpy(hostPath, hostRealPath.c_str(), size);
    }

    return hostRealPath.size();
}

size_t loader_vchroot_unexpand(const char* hostPath, char* path, size_t size)
{
    std::string hostRealPath;
    if (hostPath[0] == '/')
    {
        hostRealPath = hostPath;
    }
    else
    {
        hostRealPath = (std::filesystem::current_path() / hostPath).string();
    }

    std::vector<std::string> hostParts = GetParts(hostRealPath);

    std::string haikuPath = "/";

    if (hostParts.size() >= sHaikuPrefixParts.size() &&
        std::equal(sHaikuPrefixParts.begin(), sHaikuPrefixParts.end(),
                   hostParts.begin()))
    {
        for (size_t i = sHaikuPrefixParts.size(); i < hostParts.size(); ++i)
        {
            haikuPath += hostParts[i];
            if (i != hostParts.size() - 1)
            {
                haikuPath += "/";
            }
        }
    }
    else if (hostParts.size() > 0 && hostParts[0] == "dev")
    {
        haikuPath = hostRealPath;
    }
    else
    {
        haikuPath = kHostMountPoint;
        for (size_t i = 0; i < hostParts.size(); ++i)
        {
            haikuPath += hostParts[i];
            if (i != hostParts.size() - 1)
            {
                haikuPath += "/";
            }
        }
    }

    if (path != NULL)
    {
        strncpy(path, haikuPath.c_str(), size);
    }

    return haikuPath.size();
}

size_t loader_vchroot_expandat(int fd, const char* path, char* hostPath, size_t size)
{
    std::string haikuPath = HaikuPathFromFdAndPath(fd, path);
    return loader_vchroot_expand(haikuPath.c_str(), hostPath, size);
}

size_t loader_vchroot_unexpandat(int fd, const char* hostPath, char* path, size_t size)
{
    std::string hostRealPath;
    if (hostPath == NULL)
    {
        hostPath = "";
    }
    if (hostPath[0] == '/')
    {
        hostRealPath = hostPath;
    }
    else if (fd >= 0)
    {
        hostRealPath += HostPathFromFd(fd);
        if (hostRealPath.back() != '/')
        {
            hostRealPath += "/";
        }
        hostRealPath += hostPath;
    }
    else
    {
        // For relative paths at cwd, let loader_vchroot_unexpand handle.
        hostRealPath = hostPath;
    }
    return loader_vchroot_unexpand(hostRealPath.c_str(), path, size);
}

size_t loader_vchroot_expandlink(const char* path, char* hostPath, size_t size)
{
    std::string currentPath;
    while (true)
    {
        std::string currentHostPath(PATH_MAX, '\0');
        size_t currentHostPathLength = loader_vchroot_expand(path, currentHostPath.data(), currentHostPath.size());
        if (currentHostPathLength > currentHostPath.size())
        {
            currentHostPath.resize(currentHostPathLength);
            loader_vchroot_expand(path, currentHostPath.data(), currentHostPath.size());
        }
        else
        {
            currentHostPath.resize(currentHostPathLength);
        }

        if (!std::filesystem::is_symlink(currentHostPath))
        {
            if (hostPath != NULL)
            {
                strncpy(hostPath, currentHostPath.c_str(), size);
            }
            return currentHostPath.size();
        }

        auto linkPath = std::filesystem::read_symlink(currentHostPath);
        if (linkPath.is_relative())
        {
            currentPath = std::filesystem::path(path).parent_path() / linkPath;
        }
        else
        {
            currentPath = linkPath.string();
        }

        // Continue expanding the link.
        path = currentPath.c_str();
    }
}

size_t loader_vchroot_expandlinkat(int fd, const char* path, char* hostPath, size_t size)
{
    std::string haikuPath = HaikuPathFromFdAndPath(fd, path);
    return loader_vchroot_expand(haikuPath.c_str(), hostPath, size);
}

std::vector<std::string> GetParts(const std::string& path)
{
    assert(path[0] == '/');

    std::vector<std::string> parts;
    std::string currentPart;
    auto it = path.begin();

    while (it != path.end())
    {
        if (*it == '/')
        {
            if (currentPart == "..")
            {
                if (!parts.empty())
                {
                    parts.pop_back();
                }
                currentPart.clear();
            }
            else if (currentPart == ".")
            {
                currentPart.clear();
            }
            else if (!currentPart.empty())
            {
                parts.emplace_back();
                currentPart.swap(parts.back());
            }
        }
        else
        {
            currentPart.push_back(*it);
        }
        ++it;
    }

    if (currentPart == "..")
    {
        if (!parts.empty())
        {
            parts.pop_back();
        }
    }
    else
    {
        if (currentPart == ".")
        {
            currentPart.clear();
        }

        // Do this, even for empty "currentPart", to preserve the trailing slash.
        parts.emplace_back();
        currentPart.swap(parts.back());
    }

    return parts;
}

std::string HaikuPathFromFdAndPath(int fd, const char* path)
{
    std::string haikuPath;

    if (path == nullptr)
    {
        path = "";
    }

    if (path[0] == '/')
    {
        haikuPath = path;
    }
    else if (fd >= 0)
    {
        std::string fdPath = HostPathFromFd(fd);
        size_t length = loader_vchroot_unexpand(fdPath.c_str(), NULL, 0);
        haikuPath = std::string(length, '\0');
        loader_vchroot_unexpand(fdPath.c_str(), haikuPath.data(), length);
        if (haikuPath.back() != '/')
        {
            haikuPath += "/";
        }
        haikuPath += path;
    }
    else
    {
        // For relative paths at cwd, let loader_vchroot_expand handle.
        haikuPath = path;
    }

    return haikuPath;
}

static std::string HostPathFromFd(int fd)
{
    char fdPath[PATH_MAX];
    ssize_t linkLength = readlink(("/proc/self/fd/" + std::to_string(fd)).c_str(), fdPath, sizeof(fdPath));
    if (linkLength > 0 && linkLength < PATH_MAX)
    {
        // readlink does not append a null character for us.
        fdPath[linkLength] = '\0';
    }
    return fdPath;
}