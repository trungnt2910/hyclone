#ifndef __LOADER_REQUESTS_H__
#define __LOADER_REQUESTS_H__

#include "requests.h"

bool loader_init_requests();
intptr_t loader_handle_request(const TransferAreaRequestArgs& request);
intptr_t loader_handle_request(const InstallTeamDebuggerRequestArgs& request);

#endif // __LOADER_REQUESTS_H__