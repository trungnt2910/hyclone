#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <sys/stat.h>
#include <unordered_map>

#include "haiku_dirent.h"
#include "haiku_errors.h"
#include "haiku_stat.h"
#include "loader_readdir.h"
#include "loader_servercalls.h"
#include "loader_vchroot.h"

class LoaderDirectoryInfo
{
private:
    DIR* _handle = NULL;
    haiku_dev_t _dev;
    haiku_ino_t _ino;

public:
    LoaderDirectoryInfo(DIR* handle, haiku_dev_t dev, haiku_ino_t ino)
        : _handle(handle), _dev(dev), _ino(ino) { }
    LoaderDirectoryInfo(const LoaderDirectoryInfo&) = delete;
    LoaderDirectoryInfo(LoaderDirectoryInfo&& other)
        : _handle(other._handle), _dev(other._dev), _ino(other._ino)
    {
        other._handle = NULL;
    }
    DIR* GetHandle() const { return _handle; }
    haiku_dev_t GetDev() const { return _dev; }
    haiku_ino_t GetIno() const { return _ino; }
    ~LoaderDirectoryInfo()
    {
        if (_handle)
        {
            closedir(_handle);
            _handle = NULL;
        }
    }
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
    if (ptr != NULL)
    {
        haiku_stat st;
        loader_hserver_call_read_stat(fd, NULL, 0, false, &st, sizeof(st));
        sFdMap.emplace(fd, LoaderDirectoryInfo(ptr, st.st_dev, st.st_ino));
    }
}

void loader_closedir(int fd)
{
    std::unique_lock<std::mutex> lock(sFdMapMutex);
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

    DIR* dir = it->second.GetHandle();

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
        haikuEntry->d_pdev = it->second.GetDev();
        haikuEntry->d_pino = it->second.GetIno();
        strcpy(haikuEntry->d_name, entry->d_name);
        haikuEntry->d_reclen = haikuEntrySize;

        haikuEntrySize = (size_t)
            loader_hserver_call_transform_dirent(fd, haikuEntry, haikuEntrySize);

        if ((ssize_t)haikuEntrySize < 0)
        {
            // Skip this entry. It is probably blacklisted.
            continue;
        }

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

    rewinddir(it->second.GetHandle());
}