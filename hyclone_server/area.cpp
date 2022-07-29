#include <array>
#include <algorithm>
#include <cstring>
#include <vector>
#include <utility>

#include "haiku_area.h"
#include "haiku_errors.h"
#include "process.h"
#include "server_servercalls.h"

intptr_t server_hserver_call_register_area(hserver_context& context, void* user_area_info)
{
    haiku_area_info area_info;
    if (context.process->ReadMemory(user_area_info, &area_info, sizeof(area_info)) != sizeof(area_info))
    {
        return B_BAD_ADDRESS;
    }

    {
        auto lock = context.process->Lock();
        return context.process->RegisterArea(area_info);
    }
}

intptr_t server_hserver_call_get_area_info(hserver_context& context, int area_id, void* uesr_area_info)
{
    haiku_area_info area_info;
    {
        auto lock = context.process->Lock();
        if (context.process->IsValidAreaId(area_id))
        {
            area_info = context.process->GetArea(area_id);
        }
        else
        {
            return B_BAD_VALUE;
        }
    }

    if (context.process->WriteMemory(uesr_area_info, &area_info, sizeof(area_info)) != sizeof(area_info))
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

                // Every address here is either in the area or in the
                // unmapped range. If it's not in the unmapped range, it must
                // be in the area.
                if (addresses[i + 1] <= (uint8_t*)address || addresses[i] >= (uint8_t*)address + size)
                {
                    survivingRanges.emplace_back(addresses[i], addresses[i + 1]);
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

                for (size_t i = 1; i < survivingRanges.size(); ++i)
                {
                    haiku_area_info newArea = area;
                    newArea.address = survivingRanges[i].first;
                    newArea.size = survivingRanges[i].second - survivingRanges[i].first;
                    context.process->RegisterArea(newArea);
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