#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <thread>

#include "server_filesystem.h"
#include "server_main.h"
#include "server_prefix.h"
#include "server_workers.h"

int main(int argc, char** argv)
{
    // Just in case.
    server_remaining_workers = std::thread::hardware_concurrency();
    gHaikuPrefix = std::filesystem::canonical(getenv("HPREFIX")).string();
    std::cerr << "Setting up filesystem at " << gHaikuPrefix << "..." << std::endl;
    if (!server_setup_filesystem())
    {
        std::cerr << "failed to setup hyclone filesystem." << std::endl;
        return 1;
    }
    return server_main(argc, argv);
}