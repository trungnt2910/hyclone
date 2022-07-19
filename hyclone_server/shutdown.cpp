#include <cstdlib>
#include <iostream>

#include "haiku_errors.h"
#include "server_servercalls.h"
#include "system.h"

intptr_t server_hserver_call_shutdown(hserver_context& context, bool restart)
{
    auto& system = System::GetInstance();
    auto lock = system.Lock();

    system.Shutdown();

    if (restart)
    {
        std::cerr << "Restart not implemented" << std::endl;
        // Probably implementable using exec.
    }

    std::exit(0);
}