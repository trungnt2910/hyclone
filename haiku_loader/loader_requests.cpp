#include <cstddef>

#include "haiku_area.h"
#include "haiku_errors.h"
#include "loader_debugger.h"
#include "loader_requests.h"
#include "loader_runtime.h"
#include "loader_servercalls.h"

intptr_t loader_handle_request(const TransferAreaRequestArgs& request)
{
    using kern_clone_area_t = area_id (*)(const char *, void **, uint32, uint32, area_id);
    static kern_clone_area_t kern_clone_area = (kern_clone_area_t)loader_runtime_symbol("_kern_clone_area");

    haiku_area_info info;
    status_t status = loader_hserver_call_get_area_info(request.baseArea, &info);

    if (status != B_OK)
    {
        return status;
    }

    void* address = request.address;
    return kern_clone_area(info.name, &address, request.addressSpec, info.protection, request.transferredArea);
}

intptr_t loader_handle_request(const InstallTeamDebuggerRequestArgs& request)
{
    return loader_install_team_debugger(request.team, request.port);
}