#include <cstring>
#include <filesystem>

#include "area.h"
#include "haiku_errors.h"
#include "hsemaphore.h"
#include "process.h"
#include "server_messaging.h"
#include "server_servercalls.h"
#include "system.h"

static const int32 kMessagingAreaSize = B_PAGE_SIZE * 4;

std::shared_ptr<MessagingArea> MessagingArea::Create(const std::shared_ptr<Semaphore>& lockSem,
    const std::shared_ptr<Semaphore>& counterSem)
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

MessagingArea::~MessagingArea()
{
    if (_id >= 0)
    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        system.UnregisterArea(_id);
    }
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