#include <fstream>
#include <sstream>

#include "loader_debugger.h"

bool loader_is_debugger_present()
{
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line))
    {
        std::stringstream ss(line);
        std::string key;
        ss >> key;

        if (key == "TracerPid:")
        {
            int tracerPid;
            ss >> tracerPid;
            return tracerPid != 0;
        }
    }
    return false;
}