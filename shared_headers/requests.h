#ifndef __HYCLONE_REQUESTS_H__
#define __HYCLONE_REQUESTS_H__

#include "BeDefs.h"
#include "haiku_area.h"

enum request_id
{
    REQUEST_ID_transfer_area,
    REQUEST_ID_install_team_debugger,
};

struct RequestArgs
{
    request_id id;
};

struct TransferAreaRequestArgs : public RequestArgs
{
    area_id transferredArea;
    area_id baseArea;
    uint32 addressSpec;
    void* address;
};

struct InstallTeamDebuggerRequestArgs : public RequestArgs
{
    team_id team;
    port_id port;
};

#endif // __HYCLONE_REQUESTS_H__