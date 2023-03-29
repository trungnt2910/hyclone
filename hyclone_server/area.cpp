#include <array>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>
#include <utility>

#include "haiku_area.h"
#include "haiku_errors.h"
#include "process.h"
#include "server_servercalls.h"
#include "system.h"

intptr_t server_hserver_call_register_area(hserver_context& context, void* user_area_info)
{
    haiku_area_info area_info;
    if (context.process->ReadMemory(user_area_info, &area_info, sizeof(area_info)) != sizeof(area_info))
    {
        return B_BAD_ADDRESS;
    }

    area_info.team = context.pid;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        area_info.area = system.RegisterArea(area_info);

        if (area_info.area < 0)
        {
            return B_NO_MEMORY;
        }
    }

    if (area_info.area >= 0)
    {
        auto lock = context.process->Lock();
        if (context.process->RegisterArea(area_info.area) < 0)
        {
            return B_NO_MEMORY;
        }
    }

    return area_info.area;
}

intptr_t server_hserver_call_get_area_info(hserver_context& context, int area_id, void* user_area_info)
{
    haiku_area_info area_info;

    {
        auto& system = System::GetInstance();
        auto lock = system.Lock();
        if (system.IsValidAreaId(area_id))
        {
            area_info = system.GetArea(area_id);
        }
        else
        {
            return B_BAD_VALUE;
        }
    }

    if (context.process->WriteMemory(user_area_info, &area_info, sizeof(area_info)) != sizeof(area_info))
    {
        return B_BAD_ADDRESS;
    }

    return B_OK;
}

intptr_t server_hserver_call_get_next_area_info(hserver_context& context, int target_pid, void* user_cookie, void* user_info)
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

    haiku_area_info info;
    area_id areaId = -1;

    // TODO: Haiku stores the end of the last area in the cookie, while we store area IDs.
    // What problems can this cause?

    {
        auto lock = targetProcess->Lock();
        areaId = targetProcess->GetNextAreaIdFor(address);
        if (areaId < 0)
        {
            return B_BAD_VALUE;
        }
        info = targetProcess->GetArea(areaId);
        address = info.address;
    }

    if (context.process->WriteMemory(user_info, &info, sizeof(info)) != sizeof(info))
    {
        return B_BAD_ADDRESS;
    }

    if (context.process->WriteMemory(user_cookie, &address, sizeof(address)) != sizeof(address))
    {
        return B_BAD_ADDRESS;
    }

    return B_OK;
}

intptr_t server_hserver_call_unregister_area(hserver_context& context, int area_id)
{
    {
        auto lock = context.process->Lock();
        if (context.process->IsValidAreaId(area_id))
        {
            context.process->UnregisterArea(area_id);
            auto& system = System::GetInstance();
            auto sysLock = system.Lock();
            system.UnregisterArea(area_id);
        }
        else
        {
            return B_BAD_VALUE;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_resize_area(hserver_context& context, int area_id, size_t size)
{
    {
        auto lock = context.process->Lock();
        if (context.process->IsValidAreaId(area_id))
        {
            context.process->GetArea(area_id).size = size;
        }
        else
        {
            return B_BAD_VALUE;
        }
    }

    return B_OK;
}

intptr_t server_hserver_call_set_area_protection(hserver_context& context, int area_id, unsigned int protection)
{
    {
        auto lock = context.process->Lock();
        if (context.process->IsValidAreaId(area_id))
        {
            context.process->GetArea(area_id).protection = protection;
        }
        else
        {
            return B_BAD_VALUE;
        }
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
            auto& area = context.process->GetArea(id);

            std::array<uint8_t*, 4> addresses =
            {
                (uint8_t*)area.address,
                (uint8_t*)area.address + area.size,
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
                bool insideExistingArea = addresses[i] >= (uint8_t*)area.address && addresses[i + 1] <= (uint8_t*)area.address + area.size;

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
                area.protection = protection;
            }
            else
            {
                area.address = unchangedRanges[0].first;
                area.size = unchangedRanges[0].second - unchangedRanges[0].first;

                auto& system = System::GetInstance();
                auto sysLock = system.Lock();

                for (size_t i = 1; i < unchangedRanges.size(); ++i)
                {
                    haiku_area_info newArea = area;
                    newArea.address = unchangedRanges[i].first;
                    newArea.size = unchangedRanges[i].second - unchangedRanges[i].first;
                    newArea.area = system.RegisterArea(newArea);
                    context.process->RegisterArea(newArea.area);
                }

                for (const auto& [begin, end]: changedRanges)
                {
                    haiku_area_info newArea = area;
                    newArea.address = begin;
                    newArea.size = end - begin;
                    newArea.protection = protection;
                    newArea.area = system.RegisterArea(newArea);
                    context.process->RegisterArea(newArea.area);
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
            auto& area = context.process->GetArea(id);

            if ((uint8_t*)area.address + area.size <= (uint8_t*)address || (uint8_t*)area.address >= (uint8_t*)address + size)
            {
                continue;
            }

            // TODO: Does this lock affect the whole area or does it split the area into two like set_memory_protection?
            area.lock = type;
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
            auto& area = context.process->GetArea(id);

            std::array<uint8_t*, 4> addresses =
            {
                (uint8_t*)area.address,
                (uint8_t*)area.address + area.size,
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
                    if (addresses[i] >= (uint8_t*)area.address && addresses[i + 1] <= (uint8_t*)area.address + area.size)
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
                area.address = survivingRanges[0].first;
                area.size = survivingRanges[0].second - survivingRanges[0].first;

                auto& system = System::GetInstance();
                auto sysLock = system.Lock();

                for (size_t i = 1; i < survivingRanges.size(); ++i)
                {
                    haiku_area_info newArea = area;
                    newArea.address = survivingRanges[i].first;
                    newArea.size = survivingRanges[i].second - survivingRanges[i].first;
                    newArea.area = system.RegisterArea(newArea);
                    context.process->RegisterArea(newArea.area);
                }
            }
        }

        for (auto id : idsToUnregister)
        {
            context.process->UnregisterArea(id);
        }
    }

    return B_OK;
}