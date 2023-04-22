#ifndef __SERVER_NOTIFICATIONIMPL_H__
#define __SERVER_NOTIFICATIONIMPL_H__

#include "server_notifications.h"

class Process;
class Thread;

class TeamNotificationService : public DefaultNotificationService
{
public:
    TeamNotificationService();

    void Notify(uint32 eventCode, const std::shared_ptr<Process>& team);
};

class ThreadNotificationService : public DefaultNotificationService
{
public:
    ThreadNotificationService();

    void Notify(uint32 eventCode, team_id teamID, thread_id threadID,
        const std::shared_ptr<Thread>& thread);

    void Notify(uint32 eventCode, const std::shared_ptr<Thread>& thread);
};

#endif // __SERVER_NOTIFICATIONIMPL_H__