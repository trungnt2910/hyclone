#include <algorithm>
#include <cstdint>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <pthread.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "commpage_defs.h"
#include "haiku_area.h"
#include "haiku_fcntl.h"
#include "haiku_team.h"
#include "loader_registration.h"
#include "loader_servercalls.h"
#include "loader_vchroot.h"
#include "servercalls.h"

bool loader_register_process(int argc, char** args)
{
    haiku_team_info teamInfo;
    teamInfo.team = getpid();
    teamInfo.thread_count = 0;
    teamInfo.image_count = 0;
    teamInfo.area_count = 0;
    teamInfo.debugger_nub_thread = -1;
    teamInfo.debugger_nub_port = -1;
    teamInfo.argc = argc;

    size_t bytesWritten = 0;
    size_t bytesLeft = sizeof(teamInfo.args);
    for (char* currentArg = *args; currentArg != nullptr; currentArg = *++args) {
        if (bytesWritten != 0)
        {
            teamInfo.args[bytesWritten] = ' ';
            bytesWritten++;
            bytesLeft--;
        }
        size_t argLength = strlen(currentArg) + 1;
        argLength = std::min(argLength, bytesLeft);
        memcpy(teamInfo.args + bytesWritten, currentArg, argLength);
        bytesWritten += argLength;
        bytesLeft -= argLength;
        if (bytesLeft == 0)
            break;
    }

    teamInfo.uid = loader_hserver_call_uid_for(getuid());
    teamInfo.gid = loader_hserver_call_gid_for(getgid());

    return loader_hserver_call_register_team_info(&teamInfo) >= 0;
}

bool loader_register_builtin_areas(void* commpage, char** args)
{
    haiku_area_info areaInfo;
    memset(&areaInfo, 0, sizeof(areaInfo));
    areaInfo.team = getpid();
    areaInfo.address = commpage;
    areaInfo.size = COMMPAGE_SIZE;
    areaInfo.protection = B_READ_AREA | B_WRITE_AREA;
    areaInfo.lock = B_FULL_LOCK;
    strncpy(areaInfo.name, "commpage", sizeof(areaInfo.name));
    if (loader_hserver_call_register_area(&areaInfo, REGION_PRIVATE_MAP) < 0)
        return false;

    // Get pthread stack address and size
    pthread_attr_t attr;
    if (pthread_getattr_np(pthread_self(), &attr) == -1)
        return false;
    if (pthread_attr_getstack(&attr, &areaInfo.address, &areaInfo.size) == -1)
    {
        pthread_attr_destroy(&attr);
        return false;
    }
    pthread_attr_destroy(&attr);

    areaInfo.protection = B_READ_AREA | B_WRITE_AREA | B_STACK_AREA;
    areaInfo.lock = 0;
    std::string areaName = std::filesystem::path(*args).filename().string() + "_" + std::to_string(getpid()) + "_stack";
    strncpy(areaInfo.name, areaName.c_str(), sizeof(areaInfo.name));
    if (loader_hserver_call_register_area(&areaInfo, REGION_PRIVATE_MAP) < 0)
        return false;

    std::ifstream fin("/proc/self/maps");
    if (!fin.is_open())
        return false;
    std::string line;
    std::vector<std::tuple<uintptr_t, uintptr_t, bool>> regions;
    while (getline(fin, line))
    {
        std::stringstream ss(line);
        std::string address, perms, offset, dev, inode, pathname;
        ss >> address >> perms >> offset >> dev >> inode >> pathname;

        if (pathname.find("runtime_loader") == std::string::npos)
            continue;

        // Parse address
        std::stringstream ssAddress(address);
        std::string addressStart, addressEnd;
        getline(ssAddress, addressStart, '-');
        getline(ssAddress, addressEnd, '-');
        uintptr_t start = std::stoul(addressStart, nullptr, 16);
        uintptr_t end = std::stoul(addressEnd, nullptr, 16);
        // Parse perms
        bool isWritable = perms.find('w') != std::string::npos;
        if (regions.size() && std::get<1>(regions.back()) == start && std::get<2>(regions.back()) == isWritable)
        {
            std::get<1>(regions.back()) = end;
        }
        else
        {
            regions.push_back(std::make_tuple(start, end, isWritable));
        }
    }
    fin.close();
    for (size_t i = 0; i < regions.size(); ++i)
    {
        const auto& [start, end, isWritable] = regions[i];
        areaInfo.address = (void*)start;
        areaInfo.size = end - start;
        areaInfo.protection = B_READ_AREA;
        if (isWritable)
        {
            areaInfo.protection |= B_WRITE_AREA;
        }
        else
        {
            // Somehow this is how Haiku loads its binaries.
            // Either writable or executable.
            areaInfo.protection |= B_EXECUTE_AREA;
        }
        areaInfo.lock = 0;
        std::string areaName = std::string("runtime_loader_seg") + std::to_string(i) + (isWritable ? "rw" : "ro");
        strncpy(areaInfo.name, areaName.c_str(), sizeof(areaInfo.name));
        if (loader_hserver_call_register_area(&areaInfo, REGION_PRIVATE_MAP) < 0)
            return false;
    }

    // Still missing a certain "user area" area of size 16384.
    // Haiku uses it to store internal kernel stuff.
    // We might be able to use such an area for our "extended commpage".

    return true;
}

bool loader_register_existing_fds()
{
    int dirfd = open("/proc/self/fd", O_RDONLY | O_DIRECTORY);
    int pid = getpid();

    if (dirfd < 0)
        return false;

    std::shared_ptr<DIR> dir(fdopendir(dirfd), [](DIR* dir) { closedir(dir); });

    if (!dir)
        return false;

    std::string hycloneSockPath = std::filesystem::path(gHaikuPrefix) / HYCLONE_SOCKET_NAME;

    char path[PATH_MAX];
    char resolvedPath[PATH_MAX];

    for (int fd = 0; fd <= STDERR_FILENO; ++fd)
    {
        snprintf(path, sizeof(path), "/proc/%d/fd/%d", pid, fd);
        loader_vchroot_unexpand(path, resolvedPath, sizeof(resolvedPath));

        if (loader_hserver_call_register_fd(fd, HAIKU_AT_FDCWD, resolvedPath, sizeof(resolvedPath), false) < 0)
            return false;
    }

    struct dirent* entry;
    while ((entry = readdir(dir.get())) != NULL)
    {
        if (entry->d_type != DT_LNK)
            continue;

        int fd = atoi(entry->d_name);
        if (fd < 0)
            continue;

        if (fd == dirfd)
            continue;

        if (fd <= STDERR_FILENO)
            continue;

        path[0] = '\0';

        struct stat st;
        if (readlinkat(dirfd, entry->d_name, path, sizeof(path)) == -1 || stat(path, &st) == -1)
        {
            // Probably path is something weird such as a pipe.
            snprintf(path, sizeof(path), "/proc/%d/fd/%s", pid, entry->d_name);
        }

        loader_vchroot_unexpand(path, resolvedPath, sizeof(resolvedPath));

        if (loader_hserver_call_register_fd(fd, HAIKU_AT_FDCWD, resolvedPath, sizeof(resolvedPath), false) < 0)
            return false;
    }

    return true;
}