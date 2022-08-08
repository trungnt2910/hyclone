#ifndef __SERVER_FILESYSTEM_H__
#define __SERVER_FILESYSTEM_H__

bool server_setup_filesystem();

struct haiku_fs_info;

bool server_setup_rootfs(haiku_fs_info& info);
// Sets up the devfs "/dev" directory.
bool server_setup_devfs(haiku_fs_info& info);
// Sets up the packagefs "/boot/system" directory.
bool server_setup_packagefs(haiku_fs_info& info);
// Sets up the "/SystemRoot" directory.
bool server_setup_systemfs(haiku_fs_info& info);
// Sets up Haiku settings files.
// (The /boot/system/settings/network directory)
bool server_setup_settings();

#endif // __SERVER_FILESYSTEM_H__