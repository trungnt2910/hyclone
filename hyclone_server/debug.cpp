#include <iostream>
#include <string>

#include "haiku_errors.h"
#include "process.h"
#include "server_servercalls.h"
#include "system.h"

intptr_t server_hserver_call_debug_output(hserver_context& context, const char *userString, size_t len)
{
    std::string message(len, '\0');

    if (context.process->ReadMemory((void*)userString, message.data(), len) != len)
    {
        return B_BAD_ADDRESS;
    }

    auto& system = System::GetInstance();
    auto lock = system.Lock();

    // TODO: Make a dedicated log function
    std::cerr << "[" << context.pid << "] [" << context.tid << "] _kern_debug_output: " << message << std::endl;

    return B_OK;
}