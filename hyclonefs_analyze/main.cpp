#include <cstdlib>
#include <iostream>

#include <hyclonefs.h>

#include <fssh_dirent.h>
#include <fssh_errors.h>
#include <fssh_fs_volume.h>
#include <fssh_string.h>
#include <syscalls.h>

using namespace FSShell;

int main()
{
    const char* hprefix = getenv("HPREFIX");
    if (hprefix == NULL)
    {
        std::cerr << "HPREFIX not set" << std::endl;
        return 1;
    }
    HycloneFSArgs args;
    args.HyclonePrefix = hprefix;
    args.MountPoint = "/SystemRoot/";
    
    fssh_status_t error = hyclonefs_system_init(&args);
    
    if (error != FSSH_B_OK)
    {
        std::cerr << "hyclonefs_system_init() failed: " << fssh_strerror(error) << std::endl;
        return 1;
    }

    char dirents[1000];
    int fd = _kern_open_dir(-1, "SystemRoot");
    for (int i = 0; i < 10; ++i)
    {
        _kern_read_dir(fd, (fssh_dirent*)dirents, sizeof(dirents), 1);
        if (!fssh_strcmp(".", ((fssh_dirent*)dirents)[0].d_name))
            continue;
        if (!fssh_strcmp("..", ((fssh_dirent*)dirents)[0].d_name))
            continue;
        std::cout << ((fssh_dirent*)dirents)[0].d_name << std::endl;
    }
}