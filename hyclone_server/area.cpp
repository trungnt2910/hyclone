#include <array>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <vector>
#include <utility>

#include "area.h"
#include "haiku_area.h"
#include "haiku_errors.h"
#include "process.h"
#include "server_memory.h"
#include "server_requests.h"
#include "server_servercalls.h"
#include "server_workers.h"
#include "system.h"
#include "thread.h"

std::vector<std::shared_ptr<Area>> Area::Split(const std::vector<std::pair<uint8_t*, uint8_t*>>& ranges)
{
    std::vector<std::shared_ptr<Area>> areas;
    areas.reserve(ranges.size() - 1);

    uint8_t* originalAddress = (uint8_t*)_info.address;
    size_t originalOffset = _offset;

    _info.address = ranges[0].first;
    _info.size = ranges[0].second - ranges[0].first;
    if (IsShared())
    {
        _offset = originalOffset + (ranges[0].first - originalAddress);
    }

    for (size_t i = 1; i < ranges.size(); i++)
    {
        const auto& [start, end] = ranges[i];
        areas.emplace_back(std::make_shared<Area>(*this));
        areas.back()->_info.address = start;
        areas.back()->_info.size = end - start;
        if (IsShared())
        {
            areas.back()->_offset = originalOffset + (start - originalAddress);
        }
    }

    return areas;
}

intptr_t server_hserver_call_register_area(hserver_context& context, void* user_area_info, unsigned int mapping)
{
    haiku_area_info area_info;
    if (context.process->ReadMemory(user_area_info, &area_info, sizeof(area_info)) != sizeof(area_info))
    {
        return B_BAD_ADDRESS;
    }

    area_info.team = context.pid;

    std::shared_ptr<Area> area;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        area = system.RegisterArea(area_info).lock();
        if (!area)
        {
            return B_NO_MEMORY;
        }
    }

    {
        auto lock = context.process->Lock();
        if (!context.process->RegisterArea(area).lock())
        {
            return B_NO_MEMORY;
        }
    }

    area->SetMapping(mapping);

    return area->GetInfo().area;
}

intptr_t server_hserver_call_share_area(hserver_context& context, int areaId, intptr_t handle, size_t offset, char* path, size_t pathLen)
{
    std::shared_ptr<Area> area;

    auto& system = System::GetInstance();

    {
        auto lock = system.Lock();
        if (system.IsValidAreaId(areaId))
        {
            area = system.GetArea(areaId).lock();
        }
    }

    if (!area)
    {
        return B_BAD_VALUE;
    }

    if (handle != -1)
    {
        auto& memService = system.GetMemoryService();
        auto lock = memService.Lock();

        EntryRef ref;
        if (!memService.OpenSharedFile(context.pid, handle, area->IsWritable(), ref))
        {
            return B_ENTRY_NOT_FOUND;
        }
        area->Share(ref, offset);

        if (pathLen >= 1)
        {
            int num = 0;
            if (context.process->WriteMemory(path, &num, 1) != 1)
            {
                return B_BAD_ADDRESS;
            }
        }

        return B_OK;
    }
    else
    {
        auto& memService = system.GetMemoryService();
        auto lock = memService.Lock();

        std::string pathStr;
        if (!memService.CreateSharedFile(std::to_string(area->GetAreaId()), area->GetSize(), pathStr))
        {
            return B_NO_MEMORY;
        }

        if (pathStr.size() > pathLen)
        {
            std::filesystem::remove(pathStr);
            return B_NAME_TOO_LONG;
        }

        // If the buffer allows, write an additional NULL character to distinguish
        // the new path from one that might already be there.
        size_t writeSize = std::min(pathStr.size() + 1, pathLen);

        {
            auto procLock = context.process->Lock();
            if (context.process->WriteMemory(path, pathStr.c_str(), writeSize) != writeSize)
            {
                std::filesystem::remove(pathStr);
                return B_BAD_ADDRESS;
            }
        }

        EntryRef ref;
        if (!memService.OpenSharedFile(pathStr, area->IsWritable(), ref))
        {
            std::filesystem::remove(pathStr);
            return B_ENTRY_NOT_FOUND;
        }
        area->Share(ref, offset);

        return B_OK;
    }
}

intptr_t server_hserver_call_get_shared_area_path(hserver_context& context, int areaId, char* user_path, size_t pathLen)
{
    std::shared_ptr<Area> area;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        if (system.IsValidAreaId(areaId))
        {
            area = system.GetArea(areaId).lock();
        }
    }

    if (!area)
    {
        return B_BAD_VALUE;
    }

    if (!area->IsShared())
    {
        return B_BAD_VALUE;
    }

    auto& memService = System::GetInstance().GetMemoryService();
    auto lock = memService.Lock();

    std::string path;
    if (!memService.GetSharedFilePath(area->GetEntryRef(), path))
    {
        return B_ENTRY_NOT_FOUND;
    }

    size_t writeSize = path.size() + 1;

    if (writeSize > pathLen)
    {
        return B_BUFFER_OVERFLOW;
    }

    if (context.process->WriteMemory(user_path, path.c_str(), writeSize) != writeSize)
    {
        return B_BAD_ADDRESS;
    }

    return B_OK;
}

intptr_t server_hserver_call_transfer_area(hserver_context& context, int areaId, void** userAddress,
    unsigned int addressSpec, int baseAreaId, int target)
{
    std::shared_ptr<Area> area;
    std::shared_ptr<Area> baseArea;

    {
        auto lock = context.process->Lock();
        area = context.process->GetArea(areaId).lock();
        baseArea = context.process->GetArea(baseAreaId).lock();
    }

    if (!area)
    {
        return B_BAD_VALUE;
    }

    if (!baseArea)
    {
        return B_BAD_VALUE;
    }

    if (!area->IsShared())
    {
        return B_BAD_VALUE;
    }

    void* address;

    {
        auto lock = context.process->Lock();
        if (!context.process->ReadMemory(userAddress, &address, sizeof(address)))
        {
            return B_BAD_ADDRESS;
        }
    }

    std::shared_ptr<Thread> targetThread;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        targetThread = system.GetThread(target).lock();
    }

    if (!targetThread)
    {
        return B_BAD_VALUE;
    }

    status_t status;

    {
        auto requestLock = server_worker_run_wait([&]()
        {
            return targetThread->RequestLock();
        });

        std::shared_future<intptr_t> result;

        {
            auto lock = targetThread->Lock();

            result = targetThread->SendRequest(
                std::make_shared<TransferAreaRequest>(
                    TransferAreaRequestArgs({
                        REQUEST_ID_transfer_area,
                        areaId, baseAreaId,
                        addressSpec, address })));
        }

        status = server_worker_run_wait([&]()
        {
            return result.get();
        });
    }

    if (status != B_OK)
    {
        return status;
    }

    std::shared_ptr<Area> newArea;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        newArea = system.GetArea(status).lock();
    }

    if (!newArea)
    {
        return B_BAD_VALUE;
    }

    address = newArea->GetAddress();

    {
        auto lock = context.process->Lock();
        if (!context.process->WriteMemory(userAddress, &address, sizeof(address)))
        {
            return B_BAD_ADDRESS;
        }
    }

    return newArea->GetAreaId();
}

intptr_t server_hserver_call_get_area_info(hserver_context& context, int areaId, void* user_area_info)
{
    std::shared_ptr<Area> area;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        if (system.IsValidAreaId(areaId))
        {
            area = system.GetArea(areaId).lock();
        }
    }

    if (!area)
    {
        return B_BAD_VALUE;
    }

    if (context.process->WriteMemory(user_area_info, &area->GetInfo(), sizeof(haiku_area_info)) != sizeof(haiku_area_info))
    {
        return B_BAD_ADDRESS;
    }

    return B_OK;
}

intptr_t server_hserver_call_get_next_area_info(hserver_context& context, int target_pid, void* user_cookie, void* user_info, void* user_extended)
{
    if (target_pid == 0)
    {
        target_pid = context.pid;
    }

    std::shared_ptr<Process> targetProcess;
    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        targetProcess = system.GetProcess(target_pid).lock();
    }

    if (!targetProcess)
    {
        // Probably this means that it's a Linux process.
        std::cerr << "Process " << target_pid << " not found" << std::endl;
        std::cerr << "get_next_area_info for host processes is currently not supported." << std::endl;
        return B_BAD_VALUE;
    }

    void* address;

    if (context.process->ReadMemory(user_cookie, &address, sizeof(address)) != sizeof(address))
    {
        return B_BAD_ADDRESS;
    }

    std::shared_ptr<Area> area;
    area_id areaId = -1;

    // TODO: Haiku stores the end of the last area in the cookie, while we store area IDs.
    // What problems can this cause?

    while (true)
    {
        auto lock = targetProcess->Lock();
        areaId = targetProcess->GetNextAreaIdFor(address);
        if (areaId < 0)
        {
            return B_BAD_VALUE;
        }
        area = targetProcess->GetArea(areaId).lock();
        if (area)
        {
            address = area->GetInfo().address;
            break;
        }
    }

    if (context.process->WriteMemory(user_info, &area->GetInfo(), sizeof(haiku_area_info)) != sizeof(haiku_area_info))
    {
        return B_BAD_ADDRESS;
    }

    if (context.process->WriteMemory(user_cookie, &address, sizeof(address)) != sizeof(address))
    {
        return B_BAD_ADDRESS;
    }

    if (user_extended != NULL)
    {
        uint32_t mapping = area->GetMapping();
        if (context.process->WriteMemory(user_extended, &mapping, sizeof(mapping)) != sizeof(mapping))
        {
            return B_BAD_ADDRESS;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_unregister_area(hserver_context& context, int areaId)
{
    {
        auto lock = context.process->Lock();
        if (context.process->IsValidAreaId(areaId))
        {
            context.process->UnregisterArea(areaId);
            auto& system = System::GetInstance();
            auto sysLock = system.Lock();
            system.UnregisterArea(areaId);
        }
        else
        {
            return B_BAD_VALUE;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_resize_area(hserver_context& context, int areaId, size_t size)
{
    {
        auto lock = context.process->Lock();
        auto area = context.process->GetArea(areaId).lock();
        if (!area)
        {
            return B_BAD_VALUE;
        }
        area->GetInfo().size = size;
    }

    return B_OK;
}

intptr_t server_hserver_call_set_area_protection(hserver_context& context, int areaId, unsigned int protection)
{
    {
        auto lock = context.process->Lock();
        auto area = context.process->GetArea(areaId).lock();
        if (!area)
        {
            return B_BAD_VALUE;
        }
        area->GetInfo().protection = protection;
    }

    return B_OK;
}

intptr_t server_hserver_call_area_for(hserver_context& context, void* address)
{
    {
        auto lock = context.process->Lock();
        return context.process->GetAreaIdFor(address);
    }
}

intptr_t server_hserver_call_set_memory_protection(hserver_context& context, void* address, size_t size, unsigned int protection)
{
    {
        auto lock = context.process->Lock();
        int id = -1;
        while ((id = context.process->NextAreaId(id)) != -1)
        {
            auto area = context.process->GetArea(id).lock();

            std::array<uint8_t*, 4> addresses =
            {
                (uint8_t*)area->GetAddress(),
                (uint8_t*)area->GetAddress() + area->GetSize(),
                (uint8_t*)address,
                (uint8_t*)address + size
            };

            std::sort(addresses.begin(), addresses.end());

            std::vector<std::pair<uint8_t*, uint8_t*>> unchangedRanges;
            std::vector<std::pair<uint8_t*, uint8_t*>> changedRanges;

            for (size_t i = 0; i + 1 < addresses.size(); ++i)
            {
                if (addresses[i] == addresses[i + 1])
                {
                    continue;
                }

                bool outsideChangedRange = addresses[i + 1] <= (uint8_t*)address || addresses[i] >= (uint8_t*)address + size;
                bool insideExistingArea = addresses[i] >= (uint8_t*)area->GetAddress() && addresses[i + 1] <= (uint8_t*)area->GetAddress() + area->GetSize();

                if (insideExistingArea)
                {
                    if (outsideChangedRange)
                    {
                        unchangedRanges.emplace_back(addresses[i], addresses[i + 1]);
                    }
                    else
                    {
                        changedRanges.emplace_back(addresses[i], addresses[i + 1]);
                    }
                }
            }

            if (changedRanges.size() == 0)
            {
                // Nothing to do here, the protection hasn't changed.
                continue;
            }

            if (unchangedRanges.size() == 0)
            {
                // Everything has changed.
                area->GetInfo().protection = protection;
            }
            else
            {
                std::vector<std::pair<uint8_t*, uint8_t*>> newRanges = unchangedRanges;
                newRanges.insert(newRanges.end(), changedRanges.begin(), changedRanges.end());
                std::vector<std::shared_ptr<Area>> newAreas = area->Split(newRanges);

                assert(newAreas.size() == unchangedRanges.size() + changedRanges.size() - 1);

                auto& system = System::GetInstance();
                auto sysLock = system.Lock();

                for (size_t i = 1; i < unchangedRanges.size(); ++i)
                {
                    auto areaPtr = system.RegisterArea(newAreas[i - 1]).lock();
                    context.process->RegisterArea(areaPtr);
                }

                for (size_t i = unchangedRanges.size(); i < newAreas.size(); ++i)
                {
                    newAreas[i - 1]->GetInfo().protection = protection;
                    auto areaPtr = system.RegisterArea(newAreas[i - 1]).lock();
                    context.process->RegisterArea(areaPtr);
                }
            }
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_set_memory_lock(hserver_context& context, void* address, size_t size, int type)
{
    switch (type)
    {
        case B_NO_LOCK:
        case B_LAZY_LOCK:
        case B_FULL_LOCK:
        case B_32_BIT_FULL_LOCK:
            break;
        default:
            return B_BAD_VALUE;
    }

    {
        auto lock = context.process->Lock();
        int id = -1;
        while ((id = context.process->NextAreaId(id)) != -1)
        {
            auto area = context.process->GetArea(id).lock();

            if ((uint8_t*)area->GetAddress() + area->GetSize() <= (uint8_t*)address
                || (uint8_t*)area->GetAddress() >= (uint8_t*)address + size)
            {
                continue;
            }

            // TODO: Does this lock affect the whole area or does it split the area into two like set_memory_protection?
            area->GetInfo().lock = type;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_unmap_memory(hserver_context& context, void* address, size_t size)
{
    {
        auto lock = context.process->Lock();
        int id = -1;
        std::vector<int> idsToUnregister;
        while ((id = context.process->NextAreaId(id)) != -1)
        {
            auto area = context.process->GetArea(id).lock();

            std::array<uint8_t*, 4> addresses =
            {
                (uint8_t*)area->GetAddress(),
                (uint8_t*)area->GetAddress() + area->GetSize(),
                (uint8_t*)address,
                (uint8_t*)address + size
            };

            std::sort(addresses.begin(), addresses.end());

            std::vector<std::pair<uint8_t*, uint8_t*>> survivingRanges;

            for (size_t i = 0; i + 1 < addresses.size(); ++i)
            {
                if (addresses[i] == addresses[i + 1])
                {
                    continue;
                }

                // If the current range is outside the unmapped range...
                if (addresses[i + 1] <= (uint8_t*)address || addresses[i] >= (uint8_t*)address + size)
                {
                    // And if the current range is in the existing area...
                    if (addresses[i] >= (uint8_t*)area->GetAddress()
                        && addresses[i + 1] <= (uint8_t*)area->GetAddress() + area->GetSize())
                    {
                        survivingRanges.emplace_back(addresses[i], addresses[i + 1]);
                    }
                }
            }

            if (survivingRanges.size() == 0)
            {
                // Prevent id reuse during loop.
                idsToUnregister.push_back(id);
            }
            else
            {
                std::vector<std::shared_ptr<Area>> newAreas = area->Split(survivingRanges);

                assert(newAreas.size() == survivingRanges.size() - 1);

                auto& system = System::GetInstance();
                auto sysLock = system.Lock();

                for (size_t i = 1; i < survivingRanges.size(); ++i)
                {
                    auto areaPtr = system.RegisterArea(newAreas[i - 1]).lock();
                    context.process->RegisterArea(areaPtr);
                }
            }
        }

        {
            auto& system = System::GetInstance();
            auto sysLock = system.Lock();
            for (auto unregisterId : idsToUnregister)
            {
                context.process->UnregisterArea(unregisterId);
                system.UnregisterArea(unregisterId);
            }
        }
    }

    return B_OK;
}