#include <cstring>
#include <dirent.h>
#include <iostream>
#include <mutex>
#include <sys/stat.h>
#include <unordered_map>

#include "haiku_dirent.h"
#include "haiku_errors.h"
#include "loader_readdir.h"
#include "loader_vchroot.h"

struct LoaderDirectoryInfo
{
    DIR* handle;
    int extendedEntryIndex;
};

static std::unordered_map<int, LoaderDirectoryInfo> sFdMap;
static std::mutex sFdMapMutex;
// Currently, 16 bytes for the name is enough as we only
// need to handle SystemRoot and dev. Increase this value
// when we need to emulate more complicated paths.
static char sDirentBuf[sizeof(struct dirent) + 16];

void loader_opendir(int fd)
{
    std::unique_lock<std::mutex> lock(sFdMapMutex);
    auto ptr = fdopendir(fd);
    if (ptr != nullptr)
    {
        sFdMap[fd] = LoaderDirectoryInfo{ ptr, 0 };
    }
}

void loader_closedir(int fd)
{
    std::unique_lock<std::mutex> lock(sFdMapMutex);
    closedir(sFdMap[fd].handle);
    sFdMap.erase(fd);
}

static struct dirent* loader_readdir_extended(int fd, LoaderDirectoryInfo& info)
{
    char rootPath[4];
    // Not root.
    if (loader_vchroot_unexpandat(fd, NULL, rootPath, sizeof(rootPath)) != 1 || rootPath[0] != '/')
    {
        return NULL;
    }

    struct stat st;
    struct dirent* dirent = (struct dirent*)sDirentBuf;

    switch (info.extendedEntryIndex)
    {
        // SystemRoot
        case 0:
        {
            if (stat("/", &st))
            {
                return NULL;
            }

            dirent->d_ino = st.st_ino;
            strcpy(dirent->d_name, "SystemRoot");
        }
        break;
        // dev
        case 1:
        {
            if (stat("/dev", &st))
            {
                return NULL;
            }

            dirent->d_ino = st.st_ino;
            strcpy(dirent->d_name, "dev");
        }
        break;
        default:
            return NULL;
    }

    ++info.extendedEntryIndex;
    return dirent;
}

int loader_readdir(int fd, void* buffer, size_t bufferSize, int maxCount)
{
    std::unique_lock<std::mutex> lock(sFdMapMutex);

    auto it = sFdMap.find(fd);
    if (it == sFdMap.end())
    {
        return HAIKU_POSIX_ENOTDIR;
    }

    DIR* dir = it->second.handle;

    char* bufferOffset = (char*)buffer;
    size_t bufferSizeLeft = bufferSize;

    int count = 0;

    while (count < maxCount)
    {
        long oldPos = telldir(dir);
        struct dirent *entry = readdir(dir);
        if (entry == NULL)
        {
            entry = loader_readdir_extended(fd, it->second);
            if (entry == NULL)
            {
                break;
            }
        }
        size_t nameLen = strlen(entry->d_name);
        size_t haikuEntrySize = sizeof(haiku_dirent) + nameLen + 1;
        if (haikuEntrySize > bufferSizeLeft)
        {
            seekdir(dir, oldPos);
            break;
        }
        struct haiku_dirent* haikuEntry = (struct haiku_dirent*)bufferOffset;
        haikuEntry->d_ino = entry->d_ino;
        haikuEntry->d_pino = 0;
        haikuEntry->d_dev = 0;
        haikuEntry->d_pino = 0;
        haikuEntry->d_reclen = haikuEntrySize;
        strcpy(haikuEntry->d_name, entry->d_name);
        bufferOffset += haikuEntrySize;
        bufferSizeLeft -= haikuEntrySize;
        ++count;
    }

    return count;
}

void loader_rewinddir(int fd)
{
    std::unique_lock<std::mutex> lock(sFdMapMutex);

    auto it = sFdMap.find(fd);
    if (it == sFdMap.end())
    {
        return;
    }

    rewinddir(it->second.handle);
    it->second.extendedEntryIndex = 0;
}