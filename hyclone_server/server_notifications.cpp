// Ported from Haiku's src/system/kernel/Notifications.cpp

/*
 * Copyright 2007-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 */

#include <new>

#include "server_notifications.h"
#include "server_servercalls.h"
#include "server_systemnotification.h"
#include "system.h"

static const char* kEventMaskString = "event mask";

// #pragma mark - NotificationListener

NotificationListener::~NotificationListener()
{
}

void NotificationListener::EventOccurred(NotificationService& service,
    const KMessage* event)
{
}

void NotificationListener::AllListenersNotified(NotificationService& service)
{
}

bool NotificationListener::operator==(const NotificationListener& other) const
{
    return &other == this;
}

// #pragma mark - UserMessagingMessageSender

UserMessagingMessageSender::UserMessagingMessageSender()
    : _message(NULL), _targetCount(0)
{
}

void UserMessagingMessageSender::SendMessage(const KMessage* message, port_id port, int32 token)
{
    if ((message != _message && _message != NULL) || _targetCount == MAX_MESSAGING_TARGET_COUNT)
    {
        FlushMessage();
    }

    _message = message;
    _targets[_targetCount].port = port;
    _targets[_targetCount].token = token;
    ++_targetCount;
}

void UserMessagingMessageSender::FlushMessage()
{
    if (_message != NULL && _targetCount > 0)
    {
        auto& messagingService = System::GetInstance().GetMessagingService();
        auto lock = messagingService.Lock();
        messagingService.SendMessage(_message->Buffer(), _message->ContentSize(),
            _targets, _targetCount);
    }

    _message = NULL;
    _targetCount = 0;
}

// #pragma mark - UserMessagingListener

UserMessagingListener::UserMessagingListener(UserMessagingMessageSender& sender,
    port_id port, int32 token)
    : _sender(sender), _port(port), _token(token)
{
}

UserMessagingListener::~UserMessagingListener()
{
}

void UserMessagingListener::EventOccurred(NotificationService& service,
    const KMessage* event)
{
    _sender.SendMessage(event, _port, _token);
}

void UserMessagingListener::AllListenersNotified(NotificationService& service)
{
    _sender.FlushMessage();
}

//	#pragma mark - NotificationService

NotificationService::~NotificationService()
{
}

//	#pragma mark - default_listener

default_listener::~default_listener()
{
    // Only delete the listener if it's one of ours
    if (dynamic_cast<UserMessagingListener*>(listener) != NULL)
    {
        delete listener;
    }
}

//	#pragma mark - DefaultNotificationService

DefaultNotificationService::DefaultNotificationService(const std::string& name)
    : _name(name)
{
}

DefaultNotificationService::~DefaultNotificationService()
{
}

/*!	\brief Notifies all registered listeners.
    \param event The message defining the event
    \param eventMask Only listeners with an event mask sharing at least one
        common bit with this mask will receive the event.
*/
void DefaultNotificationService::NotifyLocked(const KMessage& event, uint32 eventMask)
{
    // Note: The following iterations support that the listener removes itself
    // in the hook method.

    // notify all listeners about the event

    for (auto it = _listeners.begin(); it != _listeners.end();)
    {
        // The iterator might be invalidated by the listener, so we need to
        // get the next one before calling the listener.
        auto nextIt = std::next(it);

        if ((eventMask & (*it)->eventMask) != 0)
        {
            (*it)->listener->EventOccurred(*this, &event);
        }

        it = nextIt;
    }

    for (auto it = _listeners.begin(); it != _listeners.end();)
    {
        auto nextIt = std::next(it);

        if ((eventMask & (*it)->eventMask) != 0)
        {
            (*it)->listener->EventOccurred(*this, &event);
        }

        it = nextIt;
    }
}

status_t DefaultNotificationService::AddListener(const KMessage* eventSpecifier,
    NotificationListener& notificationListener)
{
    if (eventSpecifier == NULL)
        return B_BAD_VALUE;

    uint32 eventMask;
    status_t status = ToEventMask(*eventSpecifier, eventMask);
    if (status != B_OK)
        return status;

    std::shared_ptr<default_listener> listener
        = std::make_shared<default_listener>();
    if (!listener)
        return B_NO_MEMORY;

    listener->eventMask = eventMask;
    listener->team = -1;
    listener->listener = &notificationListener;

    std::unique_lock<std::recursive_mutex> _(_lock);
    if (_listeners.empty())
        FirstAdded();
    _listeners.push_back(listener);

    return B_OK;
}

status_t DefaultNotificationService::UpdateListener(const KMessage* eventSpecifier,
    NotificationListener& notificationListener)
{
    return B_NOT_SUPPORTED;
}

status_t DefaultNotificationService::RemoveListener(const KMessage* eventSpecifier,
    NotificationListener& notificationListener)
{
    std::unique_lock<std::recursive_mutex> _(_lock);

    for (auto it = _listeners.begin(); it != _listeners.end();)
    {
        auto nextIt = std::next(it);

        if ((*it)->listener == &notificationListener)
        {
            _listeners.erase(it);
            if (_listeners.empty())
                LastRemoved();
            return B_OK;
        }

        it = nextIt;
    }

    return B_ENTRY_NOT_FOUND;
}

status_t DefaultNotificationService::Register()
{
    return System::GetInstance().GetNotificationManager().RegisterService(*this);
}

void DefaultNotificationService::Unregister()
{
    System::GetInstance().GetNotificationManager().UnregisterService(*this);
}

status_t DefaultNotificationService::ToEventMask(const KMessage& eventSpecifier,
                                        uint32& eventMask)
{
    return eventSpecifier.FindInt32("event mask", (int32*)&eventMask);
}

void DefaultNotificationService::FirstAdded()
{
}

void DefaultNotificationService::LastRemoved()
{
}

//	#pragma mark - DefaultUserNotificationService

DefaultUserNotificationService::DefaultUserNotificationService(const std::string& name)
    : DefaultNotificationService(name)
{
    System::GetInstance().GetNotificationManager().AddListener("teams", TEAM_REMOVED, *this);
}

DefaultUserNotificationService::~DefaultUserNotificationService()
{
    System::GetInstance().GetNotificationManager().RemoveListener("teams", NULL, *this);
}

status_t DefaultUserNotificationService::AddListener(const KMessage* eventSpecifier,
    NotificationListener& listener)
{
    if (eventSpecifier == NULL)
        return B_BAD_VALUE;

    uint32 eventMask = eventSpecifier->GetInt32(kEventMaskString, 0);

    return _AddListener(eventMask, listener);
}

status_t DefaultUserNotificationService::UpdateListener(const KMessage* eventSpecifier,
    NotificationListener& notificationListener)
{
    if (eventSpecifier == NULL)
        return B_BAD_VALUE;

    uint32 eventMask = eventSpecifier->GetInt32(kEventMaskString, 0);
    bool addEvents = eventSpecifier->GetBool("add events", false);

    std::unique_lock<std::recursive_mutex> _(_lock);

    for (auto& listener : _listeners)
    {
        if (*listener->listener == notificationListener)
        {
            if (addEvents)
                listener->eventMask |= eventMask;
            else
                listener->eventMask = eventMask;
            return B_OK;
        }
    }

    return B_ENTRY_NOT_FOUND;
}

status_t DefaultUserNotificationService::RemoveListener(const KMessage* eventSpecifier,
    NotificationListener& notificationListener)
{
    std::unique_lock<std::recursive_mutex> _(_lock);

    for (auto it = _listeners.begin(); it != _listeners.end();)
    {
        auto nextIt = std::next(it);

        if ((*it)->listener == &notificationListener)
        {
            _listeners.erase(it);
            return B_OK;
        }

        it = nextIt;
    }

    return B_ENTRY_NOT_FOUND;
}

status_t DefaultUserNotificationService::RemoveUserListeners(port_id port, uint32 token)
{
    UserMessagingListener userListener(_sender, port, token);

    std::unique_lock<std::recursive_mutex> _(_lock);

    for (auto it = _listeners.begin(); it != _listeners.end();)
    {
        auto nextIt = std::next(it);

        if (*(*it)->listener == userListener)
        {
            _listeners.erase(it);

            if (_listeners.empty())
            {
                LastRemoved();
            }

            return B_OK;
        }

        it = nextIt;
    }

    return B_ENTRY_NOT_FOUND;
}

status_t DefaultUserNotificationService::UpdateUserListener(uint32 eventMask,
    port_id port, uint32 token)
{
    UserMessagingListener userListener(_sender, port, token);

    std::unique_lock<std::recursive_mutex> _(_lock);

    for (auto& listener : _listeners)
    {
        if (*listener->listener == userListener)
        {
            listener->eventMask = eventMask;
            return B_OK;
        }
    }

    UserMessagingListener* copiedListener = new (std::nothrow) UserMessagingListener(userListener);
    if (copiedListener == NULL)
        return B_NO_MEMORY;

    status_t status = _AddListener(eventMask, *copiedListener);
    if (status != B_OK)
        delete copiedListener;

    return status;
}

void DefaultUserNotificationService::EventOccurred(NotificationService& service,
    const KMessage* event)
{
    int32 eventCode = event->GetInt32("event", -1);
    team_id team = event->GetInt32("team", -1);

    if (eventCode == TEAM_REMOVED && team >= B_OK)
    {
        // check if we have any listeners from that team, and remove them
        std::unique_lock<std::recursive_mutex> _(_lock);

        for (auto it = _listeners.begin(); it != _listeners.end();)
        {
            auto nextIt = std::next(it);

            if ((*it)->team == team)
            {
                _listeners.erase(it);
            }

            it = nextIt;
        }
    }
}

void DefaultUserNotificationService::AllListenersNotified(NotificationService& service)
{
}

status_t DefaultUserNotificationService::_AddListener(uint32 eventMask,
    NotificationListener& notificationListener)
{
    std::shared_ptr<default_listener> listener = std::make_shared<default_listener>();
    if (!listener)
    {
        return B_NO_MEMORY;
    }

    listener->eventMask = eventMask;
    listener->team = gCurrentContext->pid;
    listener->listener = &notificationListener;

    std::unique_lock<std::recursive_mutex> _(_lock);
    if (_listeners.empty())
    {
        FirstAdded();
    }
    _listeners.push_back(listener);

    return B_OK;
}

//	#pragma mark - NotificationManager

NotificationManager::NotificationManager()
{
}

NotificationManager::~NotificationManager()
{
}

NotificationService* NotificationManager::_ServiceFor(const std::string& name)
{
    if (!_services.contains(name))
    {
        return NULL;
    }
    return _services[name];
}

status_t
NotificationManager::RegisterService(NotificationService& service)
{
    std::unique_lock<std::mutex> _(_lock);

    if (_ServiceFor(service.Name()))
    {
        return B_NAME_IN_USE;
    }

    _services.emplace(service.Name(), &service);

    return B_OK;
}

void NotificationManager::UnregisterService(NotificationService& service)
{
    std::unique_lock<std::mutex> _(_lock);
    _services.erase(service.Name());
}

status_t
NotificationManager::AddListener(const std::string& serviceName,
    uint32 eventMask, NotificationListener& listener)
{
    char buffer[96];
    KMessage specifier;
    specifier.SetTo(buffer, sizeof(buffer), 0);
    specifier.AddInt32(kEventMaskString, eventMask);

    return AddListener(serviceName, &specifier, listener);
}

status_t NotificationManager::AddListener(const std::string& serviceName,
    const KMessage* eventSpecifier, NotificationListener& listener)
{
    NotificationService* service;

    {
        std::unique_lock<std::mutex> locker(_lock);
        service = _ServiceFor(serviceName);
    }

    if (service == NULL)
    {
        return B_NAME_NOT_FOUND;
    }

    return service->AddListener(eventSpecifier, listener);
}

status_t NotificationManager::UpdateListener(const std::string& serviceName,
    uint32 eventMask, NotificationListener& listener)
{
    char buffer[96];
    KMessage specifier;
    specifier.SetTo(buffer, sizeof(buffer), 0);
    specifier.AddInt32(kEventMaskString, eventMask);

    return UpdateListener(serviceName, &specifier, listener);
}

status_t NotificationManager::UpdateListener(const std::string& serviceName,
    const KMessage* eventSpecifier, NotificationListener& listener)
{
    NotificationService* service;

    {
        std::unique_lock<std::mutex> locker(_lock);
        service = _ServiceFor(serviceName);
    }

    if (service == NULL)
    {
        return B_NAME_NOT_FOUND;
    }

    return service->UpdateListener(eventSpecifier, listener);
}

status_t
NotificationManager::RemoveListener(const std::string& serviceName,
                                    const KMessage* eventSpecifier, NotificationListener& listener)
{
    NotificationService* service;

    {
        std::unique_lock<std::mutex> locker(_lock);
        service = _ServiceFor(serviceName);
    }

    if (service == NULL)
    {
        return B_NAME_NOT_FOUND;
    }

    return service->RemoveListener(eventSpecifier, listener);
}