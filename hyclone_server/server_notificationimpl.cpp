#include "kmessage.h"
#include "process.h"
#include "server_systemnotification.h"
#include "server_notificationimpl.h"
#include "thread.h"

TeamNotificationService::TeamNotificationService()
    : DefaultNotificationService("teams")
{
}

void TeamNotificationService::Notify(uint32 eventCode, const std::shared_ptr<Process>& team)
{
    char eventBuffer[128];
    KMessage event;
    event.SetTo(eventBuffer, sizeof(eventBuffer), TEAM_MONITOR);
    event.AddInt32("event", eventCode);
    event.AddInt32("team", team->GetPid());
    event.AddPointer("teamStruct", team.get());

    DefaultNotificationService::Notify(event, eventCode);
}

ThreadNotificationService::ThreadNotificationService()
    : DefaultNotificationService("threads")
{
}

void ThreadNotificationService::Notify(uint32 eventCode, team_id teamID, thread_id threadID,
            const std::shared_ptr<Thread>& thread)
{
    char eventBuffer[180];
    KMessage event;
    event.SetTo(eventBuffer, sizeof(eventBuffer), THREAD_MONITOR);
    event.AddInt32("event", eventCode);
    event.AddInt32("team", teamID);
    event.AddInt32("thread", threadID);
    if (thread)
        event.AddPointer("threadStruct", thread.get());

    DefaultNotificationService::Notify(event, eventCode);
}

void ThreadNotificationService::Notify(uint32 eventCode, const std::shared_ptr<Thread>& thread)
{
    return Notify(eventCode, thread->GetTid(), thread->GetInfo().team, thread);
}