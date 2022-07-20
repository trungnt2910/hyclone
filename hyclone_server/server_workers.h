#ifndef __SERVER_WORKERS_H__
#define __SERVER_WORKERS_H__

#include <atomic>
#include <cstddef>
#include <thread>
#include <utility>

extern std::atomic<int> server_remaining_workers;

template <typename Function, typename... Args>
void server_worker_run(Function&& func, Args&&... args)
{
    std::thread([](Function&& f, Args&&... a)
    {
        // Wait if there aren't any remaning workers.
        server_remaining_workers.wait(0);
        --server_remaining_workers;

        f(std::forward<Args&&>(a)...);

        ++server_remaining_workers;
        server_remaining_workers.notify_one();
    }, std::move(func), std::forward<Args&&>(args)...).detach();
}

// This function takes an argument in microseconds
// to match Haiku's commonly used bigtime_t.
void server_worker_sleep(uint64_t microseconds_delay);

#endif // __SERVER_WORKERS_H__