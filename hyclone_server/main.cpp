#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <thread>

#include "server_filesystem.h"
#include "server_main.h"
#include "server_memory.h"
#include "server_prefix.h"
#include "server_usermap.h"
#include "server_workers.h"
#include "system.h"

int main(int argc, char** argv)
{
    // Just in case.
    server_remaining_workers = std::thread::hardware_concurrency();
    gHaikuPrefix = std::filesystem::canonical(getenv("HPREFIX")).string();
    std::cerr << "Setting up filesystem at " << gHaikuPrefix << "..." << std::endl;
    if (!server_setup_usermap())
    {
        std::cerr << "failed to setup hyclone usermap." << std::endl;
        return 1;
    }
    if (!server_setup_filesystem())
    {
        std::cerr << "failed to setup hyclone filesystem." << std::endl;
        return 1;
    }
    if (!server_setup_memory())
    {
        std::cerr << "failed to setup hyclone memory." << std::endl;
        return 1;
    }
    auto& system = System::GetInstance();
    if (system.Init() != B_OK)
    {
        std::cerr << "failed to initialize system." << std::endl;
        return 1;
    }
    return server_main(argc, argv);
}