#include <algorithm>
#include <cstdint>
#include <cstring>
#include <unistd.h>

#include "haiku_team.h"
#include "loader_process.h"
#include "loader_servercalls.h"

bool loader_register_process(int argc, char** args)
{
    haiku_team_info teamInfo;
    teamInfo.team = getpid();
    teamInfo.thread_count = 0;
    teamInfo.image_count = 0;
    teamInfo.area_count = 0;
    teamInfo.debugger_nub_thread = -1;
    teamInfo.debugger_nub_port = -1;
    teamInfo.argc = argc;

    size_t bytesWritten = 0;
    size_t bytesLeft = sizeof(teamInfo.args);
    for (char* currentArg = *args; currentArg != nullptr; currentArg = *++args) {
        if (bytesWritten != 0)
        {
            teamInfo.args[bytesWritten] = ' ';
            bytesWritten++;
            bytesLeft--;
        }
        size_t argLength = strlen(currentArg) + 1;
        argLength = std::min(argLength, bytesLeft);
        memcpy(teamInfo.args + bytesWritten, currentArg, argLength);
        bytesWritten += argLength;
        bytesLeft -= argLength;
        if (bytesLeft == 0)
            break;
    }

    teamInfo.uid = getuid();
    teamInfo.gid = getgid();

    return loader_hserver_call_register_team_info(&teamInfo);
}