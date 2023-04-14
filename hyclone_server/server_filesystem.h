#ifndef __SERVER_FILESYSTEM_H__
#define __SERVER_FILESYSTEM_H__

#include <filesystem>

#include "fs/bindfs.h"
#include "fs/devfs.h"
#include "fs/packagefs.h"
#include "fs/rootfs.h"
#include "fs/systemfs.h"

bool server_setup_filesystem();

struct haiku_fs_info;

// Sets up Haiku prefix.
bool server_setup_prefix();

#endif // __SERVER_FILESYSTEM_H__