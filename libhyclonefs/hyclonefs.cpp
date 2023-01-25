#include <cstdio>

#include "driver_settings.h"
#include "fd.h"
#include "fssh_dirent.h"
#include "fssh_fs_volume.h"
#include "fssh_string.h"
#include "module.h"
#include "syscalls.h"
#include "vfs.h"

#include "hyclonefs.h"
#include "devfs/devfs.h"

using namespace FSShell;

extern fssh_file_system_module_info gRootFileSystem;
HycloneFSArgs* sHycloneFSArgs;

static char* hyclonefs_copy_string(const char* str)
{
    char* copy = new char[fssh_strlen(str) + 1];
    fssh_strcpy(copy, str);
    return copy;
}

int hyclonefs_system_init(const HycloneFSArgs* args)
{
    fssh_status_t error;

    // init module subsystem
    error = module_init(NULL);
    if (error != FSSH_B_OK)
    {
        fprintf(stderr, "module_init() failed: %s\n", fssh_strerror(error));
        return error;
    }

    // init driver settings
    error = driver_settings_init();
    if (error != FSSH_B_OK)
    {
        fprintf(stderr, "initializing driver settings failed: %s\n",
                fssh_strerror(error));
        return error;
    }

    // register built-in modules, i.e. the rootfs and the client FS
    register_builtin_module(&gRootFileSystem.info);
    register_builtin_module(devfs_get_module());

    // init VFS
    error = vfs_init(NULL);
    if (error != FSSH_B_OK)
    {
        fprintf(stderr, "initializing VFS failed: %s\n", fssh_strerror(error));
        return error;
    }

    // init kernel IO context
    gKernelIOContext = (io_context *)vfs_new_io_context(NULL);
    if (!gKernelIOContext)
    {
        fprintf(stderr, "creating IO context failed!\n");
        return FSSH_B_NO_MEMORY;
    }

    // mount root FS
    fssh_dev_t rootDev = _kern_mount("/", NULL, "rootfs", 0, NULL, 0);
    if (rootDev < 0)
    {
        fprintf(stderr, "mounting rootfs failed: %s\n", fssh_strerror(rootDev));
        return rootDev;
    }

    // set cwd to "/"
    error = _kern_setcwd(-1, "/");
    if (error != FSSH_B_OK)
    {
        fprintf(stderr, "setting cwd failed: %s\n", fssh_strerror(error));
        return error;
    }

    // devfs mount point
    error = _kern_create_dir(-1, "/dev", 0775);
    if (error != FSSH_B_OK)
    {
        fprintf(stderr, "creating /dev failed: %s\n", fssh_strerror(error));
        return error;
    }
    error = _kern_mount("/dev", NULL, "devfs", FSSH_B_MOUNT_VIRTUAL_DEVICE, NULL, 0);
    if (error != FSSH_B_OK)
    {
        fprintf(stderr, "mounting devfs failed: %s\n", fssh_strerror(error));
        return error;
    }

    // create mount point for the client FS
    error = _kern_create_dir(-1, args->MountPoint, 0775);
    if (error != FSSH_B_OK)
    {
        fprintf(stderr, "creating mount point failed: %s\n",
                fssh_strerror(error));
        return error;
    }

    sHycloneFSArgs = new HycloneFSArgs;
    sHycloneFSArgs->HyclonePrefix = hyclonefs_copy_string(args->HyclonePrefix);
    sHycloneFSArgs->MountPoint = hyclonefs_copy_string(args->MountPoint);

    return FSSH_B_OK;
}
