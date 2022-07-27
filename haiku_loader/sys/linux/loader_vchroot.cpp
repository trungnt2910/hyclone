#include <cassert>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <vector>
#include "loader_vchroot.h"

std::string gHaikuPrefix = "";

static const char* kHostMountPoint = "/SystemRoot/";
static std::vector<std::string> sHaikuPrefixParts;

static std::vector<std::string> GetParts(const std::string& path);

bool loader_init_vchroot(const char* hprefix)
{
    if (hprefix == NULL)
    {
        return false;
    }
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
    if (haikuRealPathParts[0] == "SystemRoot")
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
    else if (haikuRealPathParts[0] == "dev")
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

    if (std::equal(sHaikuPrefixParts.begin(), sHaikuPrefixParts.end(), hostParts.begin()))
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
    else if (hostParts[0] == "dev")
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
    std::string haikuPath;
    if (path[0] == '/')
    {
        haikuPath = path;
    }
    else if (fd >= 0)
    {
        std::string fdPath = std::filesystem::canonical(std::filesystem::path("/proc/self/fd") / std::to_string(fd)).string();
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

    return loader_vchroot_expand(haikuPath.c_str(), hostPath, size);
}

size_t loader_vchroot_unexpandat(int fd, const char* hostPath, char* path, size_t size)
{
    std::string hostRealPath;
    if (hostPath[0] == '/')
    {
        hostRealPath = hostPath;
    }
    else if (fd >= 0)
    {
        hostRealPath = std::filesystem::canonical(std::filesystem::path("/proc/self/fd") / std::to_string(fd)).string();
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