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