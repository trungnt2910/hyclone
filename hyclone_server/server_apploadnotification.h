#ifndef __SERVER_APPLOADNOTIFICATION_H__
#define __SERVER_APPLOADNOTIFICATION_H__

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

class AppLoadNotificationService
{
private:
    std::mutex _lock;
    std::unordered_map<int, std::shared_ptr<std::condition_variable>> _waitingConditions;
    std::unordered_map<int, int> _pendingNotifications;
public:
    AppLoadNotificationService() = default;
    ~AppLoadNotificationService() = default;

    int WaitForAppLoad(int pid, int64_t microsecondsTimeout);
    int NotifyAppLoad(int pid, int status, int64_t microsecondsTimeout);
};

inline const int kDefaultNotificationTimeout = 3000000;

#endif // __SERVER_APPLOADNOTIFICATION_H__
