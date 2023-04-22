#include <cstring>
#include <filesystem>
#include <iostream>

#include "area.h"
#include "haiku_errors.h"
#include "hsemaphore.h"
#include "process.h"
#include "server_messaging.h"
#include "server_servercalls.h"
#include "system.h"

static const int32 kMessagingAreaSize = B_PAGE_SIZE * 4;

MessagingArea::~MessagingArea()
{
    if (_id >= 0)
    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        {
            auto& memService = system.GetMemoryService();
            auto memLock = memService.Lock();
            memService.ReleaseMemory(_header, kMessagingAreaSize);
        }

        system.UnregisterArea(_id);
    }
}

void MessagingArea::InitHeader()
{
    _header->lock_counter = 1; // create locked
    _header->size = _size;
    _header->kernel_area = _id;
    _header->next_kernel_area = (_nextArea ? _nextArea->GetId() : -1);
    _header->command_count = 0;
    _header->first_command = 0;
    _header->last_command = 0;
}

std::shared_ptr<MessagingArea> MessagingArea::Create(const std::weak_ptr<Semaphore>& lockSem,
    const std::weak_ptr<Semaphore>& counterSem)
{
    auto area = std::shared_ptr<MessagingArea>(new MessagingArea());
    if (!area)
    {
        return area;
    }

    // TODO: Write a kernel side create_area function
    // when this code is used more often.
    std::shared_ptr<Area> messagingArea;
    haiku_area_info messagingAreaInfo;

    memset(&messagingAreaInfo, 0, sizeof(messagingAreaInfo));
    strncpy(messagingAreaInfo.name, "messaging", sizeof(messagingAreaInfo.name));
    messagingAreaInfo.size = kMessagingAreaSize;
    messagingAreaInfo.protection = B_CLONEABLE_AREA;
    messagingAreaInfo.lock = B_FULL_LOCK;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();

        messagingArea = system.RegisterArea(messagingAreaInfo).lock();
        if (!messagingArea)
        {
            return std::shared_ptr<MessagingArea>();
        }

        auto& memService = system.GetMemoryService();

        {
            auto memLock = memService.Lock();
            std::string pathStr;
            if (!memService.CreateSharedFile(
                std::to_string(messagingArea->GetAreaId()),
                messagingArea->GetSize(), pathStr))
            {
                system.UnregisterArea(messagingArea->GetAreaId());
                return std::shared_ptr<MessagingArea>();
            }

            EntryRef ref;
            if (!memService.OpenSharedFile(pathStr, true, ref))
            {
                system.UnregisterArea(messagingArea->GetAreaId());
                // This is neccessary before the file is attached to
                // the messagingArea with Share().
                std::filesystem::remove(pathStr);
                return std::shared_ptr<MessagingArea>();
            }

            messagingArea->Share(ref, 0);

            area->_header = NULL;
            area->_header = (messaging_area_header*)
                memService.AcquireMemory(ref, messagingArea->GetSize(), 0, true);

            if (!area->_header)
            {
                system.UnregisterArea(messagingArea->GetAreaId());
                return std::shared_ptr<MessagingArea>();
            }
        }
    }

    area->_id = messagingArea->GetAreaId();
    area->_size = messagingAreaInfo.size;
    area->_lockSem = lockSem;
    area->_counterSem = counterSem;
    area->InitHeader();

    return area;
}

bool MessagingArea::CheckCommandSize(int32 dataSize)
{
    int32 size = sizeof(messaging_command) + dataSize;

    return (dataSize >= 0 &&
        size <= kMessagingAreaSize - (int32)sizeof(messaging_area_header));
}

bool MessagingArea::Lock()
{
    // benaphore-like locking
    if (_header->lock_counter.fetch_add(1) == 0)
    {
        return true;
    }

    auto lockSem = _lockSem.lock();
    if (!lockSem)
    {
        return false;
    }

    // TODO: Remove the confusing tid argument from Acquire?
    return lockSem->Acquire(0, 1) == B_OK;
}

void MessagingArea::Unlock()
{
    if (_header->lock_counter.fetch_sub(1) > 1)
    {
        auto lockSem = _lockSem.lock();
        if (lockSem)
        {
            lockSem->Release(1);
        }
    }
}

void* MessagingArea::AllocateCommand(uint32 commandWhat, int32 dataSize,
    bool& wasEmpty)
{
    int32 size = sizeof(messaging_command) + dataSize;

    if (dataSize < 0 || size > _size - (int32)sizeof(messaging_area_header))
        return NULL;

    // the area is used as a ring buffer
    int32 startOffset = sizeof(messaging_area_header);

    // the simple case first: the area is empty
    int32 commandOffset;
    wasEmpty = (_header->command_count == 0);
    if (wasEmpty)
    {
        commandOffset = startOffset;

        // update the header
        _header->command_count++;
        _header->first_command = _header->last_command = commandOffset;
    }
    else
    {
        int32 firstCommandOffset = _header->first_command;
        int32 lastCommandOffset = _header->last_command;
        int32 firstCommandSize;
        int32 lastCommandSize;
        messaging_command *firstCommand = _CheckCommand(firstCommandOffset,
                                                        firstCommandSize);
        messaging_command *lastCommand = _CheckCommand(lastCommandOffset,
                                                       lastCommandSize);
        if (!firstCommand || !lastCommand)
        {
            // something has been screwed up
            return NULL;
        }

        // find space for the command
        if (firstCommandOffset <= lastCommandOffset)
        {
            // not wrapped
            // try to allocate after the last command
            if (size <= _size - (lastCommandOffset + lastCommandSize))
            {
                commandOffset = (lastCommandOffset + lastCommandSize);
            }
            else
            {
                // is there enough space before the first command?
                if (size > firstCommandOffset - startOffset)
                    return NULL;
                commandOffset = startOffset;
            }
        }
        else
        {
            // wrapped: we can only allocate between the last and the first
            // command
            commandOffset = lastCommandOffset + lastCommandSize;
            if (size > firstCommandOffset - commandOffset)
                return NULL;
        }

        // update the header and the last command
        _header->command_count++;
        lastCommand->next_command = _header->last_command = commandOffset;
    }

    // init the command
    messaging_command* command = (messaging_command*)((uint8_t*)_header + commandOffset);
    command->next_command = 0;
    command->command = commandWhat;
    command->size = size;

    return command->data;
}

void MessagingArea::CommitCommand()
{
    auto lockSem = _lockSem.lock();

    if (lockSem)
    {
        lockSem->Release(1);
    }
}

messaging_command* MessagingArea::_CheckCommand(int32 offset, int32 &size)
{
    // check offset
    if (offset < (int32)sizeof(messaging_area_header)
        || offset + (int32)sizeof(messaging_command) > _size
        || (offset & 0x3))
    {
        return NULL;
    }

    // get and check size
    messaging_command* command = (messaging_command*)((uint8_t*)_header + offset);
    size = command->size;
    if (size < (int32)sizeof(messaging_command))
    {
        return NULL;
    }
    size = (size + 3) & ~0x3; // align
    if (offset + size > _size)
        return NULL;

    return command;
}

status_t MessagingService::RegisterService(const std::shared_ptr<Process>& team,
    const std::shared_ptr<Semaphore>& lockSem,
    const std::shared_ptr<Semaphore>& counterSem,
    area_id& areaId)
{
    if (_firstArea)
    {
        return B_BAD_VALUE;
    }

    if (team->GetPid() != lockSem->GetOwningTeam()
        || team->GetPid() != counterSem->GetOwningTeam())
    {
        return B_BAD_VALUE;
    }

    _firstArea = _lastArea = MessagingArea::Create(lockSem, counterSem);
    if (!_firstArea)
    {
        return B_NO_MEMORY;
    }

    areaId = _firstArea->GetId();
    _firstArea->Unlock();

    _serverTeam = team;
    _lockSem = lockSem;
    _counterSem = counterSem;

    return B_OK;
}

status_t MessagingService::UnregisterService(const std::shared_ptr<Process>& team)
{
    if (team != _serverTeam.lock())
        return B_BAD_VALUE;

    // delete all areas
    while (_firstArea)
    {
        auto area = _firstArea;
        _firstArea = area->NextArea();
    }
    _lastArea = std::shared_ptr<MessagingArea>();

    // unset the other members
    _lockSem = std::weak_ptr<Semaphore>();
    _counterSem = std::weak_ptr<Semaphore>();
    _serverTeam = std::weak_ptr<Process>();

    return B_OK;
}

status_t MessagingService::SendMessage(const void* message, int32 messageSize,
    const messaging_target* targets, int32 targetCount)
{
    if (!message || messageSize <= 0 || !targets || targetCount <= 0)
    {
        return B_BAD_VALUE;
    }

    int32 dataSize = sizeof(messaging_command_send_message)
        + targetCount * sizeof(messaging_target) + messageSize;

    // allocate space for the command
    std::shared_ptr<MessagingArea> area;
    void* data;
    bool wasEmpty;
    status_t error = _AllocateCommand(MESSAGING_COMMAND_SEND_MESSAGE, dataSize,
        area, data, wasEmpty);
    if (error != B_OK)
    {
        std::cerr << "MessagingService::SendMessage(): Failed to allocate space for "
            "send message command.\n" << std::endl;
        return error;
    }

    // prepare the command
    messaging_command_send_message* command = (messaging_command_send_message*)data;
    command->message_size = messageSize;
    command->target_count = targetCount;
    memcpy(command->targets, targets, sizeof(messaging_target) * targetCount);
    memcpy((char*)command + (dataSize - messageSize), message, messageSize);

    // shoot
    area->Unlock();
    if (wasEmpty)
    {
        area->CommitCommand();
    }

    return B_OK;
}

status_t MessagingService::_AllocateCommand(int32 commandWhat, int32 size,
    std::shared_ptr<MessagingArea>& area, void*& data, bool& wasEmpty)
{
    if (!_firstArea)
    {
        return B_NO_INIT;
    }

    if (!MessagingArea::CheckCommandSize(size))
    {
        return B_BAD_VALUE;
    }

    // delete the discarded areas (save one)
    std::shared_ptr<MessagingArea> discardedArea = NULL;

    while (_firstArea != _lastArea)
    {
        area = _firstArea;
        area->Lock();
        if (!area->IsEmpty())
        {
            area->Unlock();
            break;
        }

        _firstArea = area->NextArea();
        area->SetNextArea(NULL);
        discardedArea = area;
    }

    // allocate space for the command in the last area
    area = _lastArea;
    area->Lock();
    data = area->AllocateCommand(commandWhat, size, wasEmpty);

    if (!data)
    {
        // not enough space in the last area: create a new area or reuse a
        // discarded one
        if (discardedArea)
        {
            area = discardedArea;
            area->InitHeader();
        }
        else
        {
            area = MessagingArea::Create(_lockSem, _counterSem);
        }
        if (!area)
        {
            _lastArea->Unlock();
            return B_NO_MEMORY;
        }

        // add the new area
        _lastArea->SetNextArea(area);
        _lastArea->Unlock();
        _lastArea = area;

        // allocate space for the command
        data = area->AllocateCommand(commandWhat, size, wasEmpty);

        if (!data)
        {
            // that should never happen
            area->Unlock();
            return B_NO_MEMORY;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_register_messaging_service(hserver_context& context, int lockSemId, int counterSemId)
{
    auto& system = System::GetInstance();
    auto& msgService = system.GetMessagingService();

    std::shared_ptr<Semaphore> lockSem, counterSem;

    {
        auto lock = system.Lock();

        lockSem = system.GetSemaphore(lockSemId).lock();
        counterSem = system.GetSemaphore(counterSemId).lock();

        if (!lockSem || !counterSem)
        {
            return B_BAD_SEM_ID;
        }
    }

    auto msgLock = msgService.Lock();

    area_id area = 0;
    status_t error = msgService.RegisterService(context.process, lockSem, counterSem, area);

    if (error != B_OK)
    {
        return error;
    }

    return area;
}

intptr_t server_hserver_call_unregister_messaging_service(hserver_context& context)
{
    auto& system = System::GetInstance();
    auto& msgService = system.GetMessagingService();

    auto msgLock = msgService.Lock();

    return msgService.UnregisterService(context.process);
}