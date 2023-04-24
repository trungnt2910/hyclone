#include "haiku_errors.h"
#include "server_apploadnotification.h"
#include "server_servercalls.h"
#include "server_time.h"
#include "server_workers.h"
#include "system.h"

int AppLoadNotificationService::WaitForAppLoad(int pid, int64_t microsecondsTimeout)
{
    std::shared_ptr<std::condition_variable> cond;
    {
        auto lock = std::unique_lock<std::mutex>(_lock);
        if (_pendingNotifications.contains(pid))
        {
            int status = _pendingNotifications[pid];
            _pendingNotifications.erase(pid);
            return status;
        }

        cond = _waitingConditions[pid];
        if (!cond)
        {
            cond = std::make_shared<std::condition_variable>();
            _waitingConditions[pid] = cond;
        }
    }

    int status = HAIKU_POSIX_ESRCH;

    {
        auto lock = std::unique_lock<std::mutex>(_lock);

        bool success = false;

        server_worker_run_wait([&]()
        {
            if (!server_is_infinite_timeout(microsecondsTimeout))
            {
                success = cond->wait_for(
                    lock,
                    std::chrono::microseconds(microsecondsTimeout),
                    [&](){return _pendingNotifications.contains(pid);});
            }
            else
            {
                cond->wait(lock, [&](){return _pendingNotifications.contains(pid);});
                success = true;
            }
        });

        if (success)
        {
            status = _pendingNotifications[pid];
            _pendingNotifications.erase(pid);
        }
    }

    {
        auto lock = std::unique_lock<std::mutex>(_lock);
        _waitingConditions.erase(pid);
    }

    return status;
}

int AppLoadNotificationService::NotifyAppLoad(int pid, int status, int64_t microsecondsTimeout)
{
    {
        auto lock = std::unique_lock<std::mutex>(_lock);
        _pendingNotifications[pid] = status;
    }

    while (microsecondsTimeout > 0)
    {
        {
            auto lock = std::unique_lock<std::mutex>(_lock);

            // The notification has been collected.
            if (!_pendingNotifications.contains(pid))
            {
                break;
            }

            if (_waitingConditions.contains(pid))
            {
                auto cond = _waitingConditions[pid];
                cond->notify_all();
                return 0;
            }
        }

        microsecondsTimeout -= 100 * 1000;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    {
        auto lock = std::unique_lock<std::mutex>(_lock);
        _pendingNotifications.erase(pid);
    }

    return HAIKU_POSIX_ESRCH;
}

intptr_t server_hserver_call_notify_loading_app(hserver_context& context, int status)
{
    server_worker_run([](int pid)
    {
        auto& service = System::GetInstance().GetAppLoadNotificationService();
        service.NotifyAppLoad(pid, B_OK, kDefaultNotificationTimeout);
    }, std::move(context.pid));

    return B_OK;
}

intptr_t server_hserver_call_wait_for_app_load(hserver_context& context, int pid)
{
    auto& service = System::GetInstance().GetAppLoadNotificationService();
    return service.WaitForAppLoad(pid, kDefaultNotificationTimeout);
}
