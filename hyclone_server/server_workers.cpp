#include <thread>
#include "server_workers.h"

std::atomic<int> server_remaining_workers = std::thread::hardware_concurrency();

void server_worker_sleep(uint64_t microseconds_delay)
{
    ++server_remaining_workers;
    server_remaining_workers.notify_one();

    std::this_thread::sleep_for(std::chrono::microseconds(microseconds_delay));

    server_remaining_workers.wait(0);
    --server_remaining_workers;
}