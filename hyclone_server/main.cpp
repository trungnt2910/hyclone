#include <thread>

#include "server_main.h"
#include "server_workers.h"

int main(int argc, char** argv)
{
    // Just in case.
    server_remaining_workers = std::thread::hardware_concurrency();
    return server_main(argc, argv);
}