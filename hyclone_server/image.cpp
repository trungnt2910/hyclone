#include <iostream>
#include <mutex>

#include "haiku_errors.h"
#include "haiku_image.h"
#include "process.h"
#include "server_servercalls.h"
#include "system.h"

intptr_t server_hserver_call_register_image(hserver_context& context, void* image_info, std::size_t size)
{
    if (size != sizeof(haiku_extended_image_info))
    {
        return B_BAD_VALUE;
    }

    haiku_extended_image_info info;
    if (context.process->ReadMemory(image_info, &info, sizeof(info)) != sizeof(info))
    {
        // Probably due to a bad pointer.
        return B_BAD_ADDRESS;
    }

    {
        auto lock = context.process->Lock();
        return context.process->RegisterImage(info);
    }
}

intptr_t server_hserver_call_get_next_image_info(hserver_context& context,
    int target_pid, int* user_cookie, void* user_image_info, std::size_t size)
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
        std::cerr << "get_next_image_info for host processes is currently not supported." << std::endl;
        return B_BAD_VALUE;
    }

    if (size != sizeof(haiku_image_info))
    {
        return B_BAD_VALUE;
    }

    int image_id;

    if (context.process->ReadMemory(user_cookie, &image_id, sizeof(image_id)) != sizeof(image_id))
    {
        return B_BAD_ADDRESS;
    }

    if (image_id < 0)
    {
        return B_BAD_VALUE;
    }

    haiku_image_info info;

    {
        auto lock = targetProcess->Lock();
        if (!targetProcess->IsValidImageId(image_id))
        {
            image_id = targetProcess->NextImageId(image_id);
        }
        // It may pass the end.
        if (image_id < 0)
        {
            return B_BAD_VALUE;
        }
        info = targetProcess->GetImage(image_id).basic_info;
        image_id = targetProcess->NextImageId(image_id);
    }

    if (context.process->WriteMemory(user_image_info, &info, sizeof(info)) != sizeof(info))
    {
        return B_BAD_ADDRESS;
    }

    if (context.process->WriteMemory(user_cookie, &image_id, sizeof(image_id)) != sizeof(image_id))
    {
        return B_BAD_ADDRESS;
    }


    return B_OK;
}

intptr_t server_hserver_call_unregister_image(hserver_context& context, int image_id)
{
    auto lock = context.process->Lock();
    context.process->UnregisterImage(image_id);
    return B_OK;
}

intptr_t server_hserver_call_image_relocated(hserver_context& context, int image_id)
{
    std::cerr << "[" << context.pid << "/" << context.tid << "]" << "stub: server_hserver_call_image_relocated" << std::endl;
    return B_OK;
}