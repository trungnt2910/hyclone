// Ported from Haiku's headers/private/kernel/Notifications.h

/*
 * Copyright 2007-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 */
#ifndef __SERVER_NOTIFICATION_H__
#define __SERVER_NOTIFICATION_H__

#include <list>
#include <mutex>
#include <unordered_map>

#include "BeDefs.h"
#include "kmessage.h"
#include "server_messaging.h"

class NotificationService;

class NotificationListener
{
public:
    virtual ~NotificationListener();

    virtual void EventOccurred(NotificationService& service, const KMessage* event);
    virtual void AllListenersNotified(NotificationService& service);

    virtual bool operator==(const NotificationListener& other) const;

    bool operator!=(const NotificationListener& other) const { return !(*this == other); }
};

class UserMessagingMessageSender
{
private:
    enum
    {
        MAX_MESSAGING_TARGET_COUNT = 16,
    };

    const KMessage* _message;
    messaging_target _targets[MAX_MESSAGING_TARGET_COUNT];
    int _targetCount;
public:
    UserMessagingMessageSender();

    void SendMessage(const KMessage* message, port_id port, int token);
    void FlushMessage();
};

class UserMessagingListener : public NotificationListener
{
private:
    UserMessagingMessageSender& _sender;
    port_id _port;
    int _token;
public:
    UserMessagingListener(UserMessagingMessageSender& sender, port_id port, int token);
    virtual ~UserMessagingListener();

    virtual void EventOccurred(NotificationService& service, const KMessage* event);
    virtual void AllListenersNotified(NotificationService& service);

    port_id Port() const { return _port; }
    int32 Token() const { return _token; }

    bool operator==(const NotificationListener& _other) const;
};

inline bool
UserMessagingListener::operator==(const NotificationListener& _other) const
{
    const UserMessagingListener* other = dynamic_cast<const UserMessagingListener*>(&_other);
    return other != NULL && other->Port() == Port() && other->Token() == Token();
}

class NotificationService : public std::enable_shared_from_this<NotificationService>
{
private:
    std::shared_ptr<NotificationService> _link;
public:
    virtual ~NotificationService();

    virtual status_t AddListener(const KMessage* eventSpecifier,
        NotificationListener& listener) = 0;
    virtual status_t RemoveListener(const KMessage* eventSpecifier,
        NotificationListener& listener) = 0;
    virtual status_t UpdateListener(const KMessage* eventSpecifier,
        NotificationListener& listener) = 0;

    virtual const std::string& Name() = 0;
    std::shared_ptr<NotificationService>& Link() { return _link; }
};

struct default_listener
{
    uint32 eventMask;
    team_id team;
    NotificationListener* listener;

    ~default_listener();
};

typedef std::list<std::shared_ptr<default_listener>> DefaultListenerList;

class DefaultNotificationService : public NotificationService
{
protected:
    virtual status_t ToEventMask(const KMessage& eventSpecifier, uint32& eventMask);
    virtual void FirstAdded();
    virtual void LastRemoved();

    std::recursive_mutex _lock;
    DefaultListenerList _listeners;
    std::string _name;
public:
    DefaultNotificationService(const std::string& name);
    virtual ~DefaultNotificationService();

    std::unique_lock<std::recursive_mutex> Lock()
        { return std::unique_lock<std::recursive_mutex>(_lock); }

    inline void Notify(const KMessage& event, uint32 eventMask);
    void NotifyLocked(const KMessage& event, uint32 eventMask);

    inline bool HasListeners() const { return !_listeners.empty(); }
    virtual status_t AddListener(const KMessage* eventSpecifier,
        NotificationListener& listener);
    virtual status_t UpdateListener(const KMessage* eventSpecifier,
        NotificationListener& listener);
    virtual status_t RemoveListener(const KMessage* eventSpecifier,
        NotificationListener& listener);

    virtual const std::string& Name() { return _name; }

    status_t Register();
    void Unregister();
};

class DefaultUserNotificationService : public DefaultNotificationService, NotificationListener
{
private:
    virtual void EventOccurred(NotificationService& service,
                               const KMessage* event);
    virtual void AllListenersNotified(NotificationService& service);
    status_t _AddListener(uint32 eventMask, NotificationListener& listener);

    UserMessagingMessageSender _sender;
public:
    DefaultUserNotificationService(const std::string& name);
    virtual ~DefaultUserNotificationService();

    virtual status_t AddListener(const KMessage* eventSpecifier,
        NotificationListener& listener);
    virtual status_t UpdateListener(const KMessage* eventSpecifier,
        NotificationListener& listener);
    virtual status_t RemoveListener(const KMessage* eventSpecifier,
        NotificationListener& listener);

    status_t RemoveUserListeners(port_id port, uint32 token);
    status_t UpdateUserListener(uint32 eventMask, port_id port, uint32 token);
};

class NotificationManager
{
    NotificationService* _ServiceFor(const std::string& name);

    static NotificationManager _manager;

    std::mutex _lock;
    std::unordered_map<std::string, NotificationService*> _services;
public:
    NotificationManager();
    ~NotificationManager();

    status_t RegisterService(NotificationService& service);
    void UnregisterService(NotificationService& service);

    status_t AddListener(const std::string& service, uint32 eventMask,
        NotificationListener& listener);
    status_t AddListener(const std::string& service, const KMessage* eventSpecifier,
        NotificationListener& listener);

    status_t UpdateListener(const std::string& service, uint32 eventMask,
        NotificationListener& listener);
    status_t UpdateListener(const std::string& service,
        const KMessage* eventSpecifier, NotificationListener& listener);

    status_t RemoveListener(const std::string& service,
        const KMessage* eventSpecifier, NotificationListener& listener);
};

void DefaultNotificationService::Notify(const KMessage& event, uint32 eventMask)
{
    std::unique_lock<std::recursive_mutex> _(_lock);
    NotifyLocked(event, eventMask);
}

#endif // __SERVER_NOTIFICATION_H__