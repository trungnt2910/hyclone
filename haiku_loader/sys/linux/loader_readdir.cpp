#include <cstring>
#include <dirent.h>
#include <iostream>
#include <mutex>
#include <unordered_map>

#include "haiku_dirent.h"
#include "haiku_errors.h"
#include "loader_readdir.h"

static std::unordered_map<int, void*> sFdMap;
static std::mutex sFdMapMutex;

void loader_opendir(int fd)
{
    std::unique_lock<std::mutex> lock(sFdMapMutex);
    auto ptr = fdopendir(fd);
    if (ptr != nullptr)
    {
        sFdMap[fd] = ptr;
    }
}

void loader_closedir(int fd)
{
    std::unique_lock<std::mutex> lock(sFdMapMutex);
    closedir((DIR*)sFdMap[fd]);
    sFdMap.erase(fd);
}

int loader_readdir(int fd, void* buffer, size_t bufferSize, int maxCount)
{
    std::unique_lock<std::mutex> lock(sFdMapMutex);

    auto it = sFdMap.find(fd);
    if (it == sFdMap.end())
    {
        return HAIKU_POSIX_ENOTDIR;
    }

    DIR* dir = (DIR*)it->second;

    char* bufferOffset = (char*)buffer;
    size_t bufferSizeLeft = bufferSize;

    int count = 0;

    while (count < maxCount)
    {
        long oldPos = telldir(dir);
        struct dirent *entry = readdir(dir);
        if (entry == NULL)
        {
            break;
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