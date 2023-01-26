#include <cstdlib>
#include <iostream>

#include <hyclonefs.h>

#include <fssh_dirent.h>
#include <fssh_errors.h>
#include <fssh_fcntl.h>
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
        error = _kern_read_dir(fd, (fssh_dirent*)dirents, sizeof(dirents), 1);
        if (!fssh_strcmp(".", ((fssh_dirent*)dirents)[0].d_name))
            continue;
        if (!fssh_strcmp("..", ((fssh_dirent*)dirents)[0].d_name))
            continue;
        if (error == 0)
            break;
        std::cout << ((fssh_dirent*)dirents)[0].d_name << std::endl;
    }

    fd = _kern_open_dir(-1, "dev");
    if (fd < 0)
    {
        std::cerr << "open_dir(\"dev\") failed: " << fssh_strerror(fd) << std::endl;
        return 1;
    }
    for (int i = 0; i < 10; ++i)
    {
        error = _kern_read_dir(fd, (fssh_dirent*)dirents, sizeof(dirents), 1);
        // if (!fssh_strcmp(".", ((fssh_dirent*)dirents)[0].d_name))
        //     continue;
        // if (!fssh_strcmp("..", ((fssh_dirent*)dirents)[0].d_name))
        //     continue;
        if (error == 0)
            break;
        std::cout << ((fssh_dirent*)dirents)[0].d_name << std::endl;
    }
    _kern_close(fd);

    fd = _kern_open(-1, "dev/random", FSSH_B_READ_WRITE, 0);
    if (fd < 0)
    {
        std::cerr << "open(\"dev/random\") failed: " << fssh_strerror(fd) << std::endl;
        return 1;
    }
    
    char buf[16];
    error = _kern_read(fd, 0, buf, sizeof(buf));

    if (error < 0)
    {
        std::cerr << "read(\"dev/random\") failed: " << fssh_strerror(error) << std::endl;
        return 1;
    }
    else
    {
        std::cout << "random bytes: ";
        for (int i = 0; i < 16; ++i)
        {
            std::cout << (int)buf[i] << " ";
        }
        std::cout << std::endl;
    }

    error = _kern_write(fd, 0, "hello", 5);
    if (error < 0)
    {
        std::cerr << "write(\"dev/random\") failed: " << fssh_strerror(error) << std::endl;
        return 1;
    }
    else
    {
        std::cout << "wrote " << error << " bytes to \"dev/random\"" << std::endl;
    }

    _kern_close(fd);

}
