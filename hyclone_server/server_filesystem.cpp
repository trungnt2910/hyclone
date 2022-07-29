#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "haiku_errors.h"
#include "haiku_fs_info.h"
#include "process.h"
#include "server_filesystem.h"
#include "server_prefix.h"
#include "server_servercalls.h"
#include "system.h"

bool server_setup_filesystem()
{
    if (gHaikuPrefix.empty())
    {
        std::cerr << "HPREFIX not set." << std::endl;
        return false;
    }

    auto& system = System::GetInstance();
    auto lock = system.Lock();

    haiku_fs_info info;

    if (!server_setup_rootfs(info))
    {
        std::cerr << "failed to setup hyclone rootfs." << std::endl;
        return false;
    }

    // dev = 0
    system.RegisterFSInfo(std::make_shared<haiku_fs_info>(info));

    if (!server_setup_devfs(info))
    {
        std::cerr << "failed to setup hyclone devfs." << std::endl;
        return false;
    }

    // dev = 1
    system.RegisterFSInfo(std::make_shared<haiku_fs_info>(info));

    if (!server_setup_packagefs(info))
    {
        std::cerr << "failed to setup hyclone packagefs." << std::endl;
        return false;
    }

    // dev = 2
    system.RegisterFSInfo(std::make_shared<haiku_fs_info>(info));

    if (!server_setup_systemfs(info))
    {
        std::cerr << "failed to setup hyclone systemfs." << std::endl;
        return false;
    }

    // dev = 3
    system.RegisterFSInfo(std::make_shared<haiku_fs_info>(info));

    return true;
}

intptr_t server_hserver_call_read_fs_info(hserver_context& context, int deviceId, void* info)
{
    auto& system = System::GetInstance();

    std::shared_ptr<haiku_fs_info> fsInfo;

    {
        auto lock = system.Lock();

        fsInfo = system.FindFSInfoByDevId(deviceId).lock();
        if (!fsInfo)
        {
            return B_DEVICE_NOT_FOUND;
        }
    }

    if (context.process->WriteMemory(info, fsInfo.get(), sizeof(haiku_fs_info)) != sizeof(haiku_fs_info))
    {
        return B_BAD_ADDRESS;
    }

    return B_OK;
}

// Sample filesystem info of a Haiku machine.
// ~/Desktop> ./statfs
// dev: 1
// root: 1
// flags: 0
// block_size: 0
// io_size: 0
// total_blocks: 0
// free_blocks: 0
// total_nodes: 0
// free_nodes: 0
// device_name:
// volume_name:
// fsh_name: rootfs
// ==============
// dev: 2
// root: 0
// flags: 0
// block_size: 0
// io_size: 0
// total_blocks: 0
// free_blocks: 0
// total_nodes: 0
// free_nodes: 0
// device_name:
// volume_name:
// fsh_name: devfs
// ==============
// dev: 3
// root: 524288
// flags: 4653060
// block_size: 2048
// io_size: 65536
// total_blocks: 16776704
// free_blocks: 7638557
// total_nodes: 0
// free_nodes: 0
// device_name: /dev/disk/nvme/0/0
// volume_name: Haiku_SSD
// fsh_name: bfs
// ==============
// dev: 4
// root: 1
// flags: 2555909
// block_size: 4096
// io_size: 65536
// total_blocks: 1
// free_blocks: 1
// total_nodes: 0
// free_nodes: 0
// device_name:
// volume_name: system
// fsh_name: packagefs
// ==============
// dev: 5
// root: 1
// flags: 2555909
// block_size: 4096
// io_size: 65536
// total_blocks: 1
// free_blocks: 1
// total_nodes: 0
// free_nodes: 0
// device_name:
// volume_name: config
// fsh_name: packagefs
// ==============
// dev: 6
// root: 524288
// flags: 4653060
// block_size: 2048
// io_size: 65536
// total_blocks: 12582912
// free_blocks: 8767256
// total_nodes: 0
// free_nodes: 0
// device_name: /dev/disk/ata/0/slave/1
// volume_name: Data
// fsh_name: bfs
// ==============
