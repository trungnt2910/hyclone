#include <algorithm>
#include <climits>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <spawn.h>
#include <string>
#include <unistd.h>
#include "loader_debugger.h"
#include "loader_servercalls.h"
#include "loader_spawn.h"
#include "loader_vchroot.h"

int loader_spawn(const char* path, const char* const* flatArgs,
    size_t flatArgsSize, int argc, int envc,
    int priority, int flags, int errorPort, int errorToken)
{
    const int ADDITIONAL_ARGS = 10;

    const char** argv = new const char*[argc + ADDITIONAL_ARGS + 1];
    const std::string loaderPath = std::filesystem::canonical("/proc/self/exe").string();
    const std::string errorPortValue = std::to_string(errorPort);
    const std::string errorTokenValue = std::to_string(errorToken);

    char cwd[PATH_MAX];
    loader_hserver_call_getcwd(cwd, sizeof(cwd));

    argv[0] = loaderPath.c_str();
    argv[1] = "--error-port";
    argv[2] = errorPortValue.c_str();
    argv[3] = "--error-token";
    argv[4] = errorTokenValue.c_str();
    argv[5] = "--prefix";
    argv[6] = gHaikuPrefix.c_str();
    argv[7] = "--cwd";
    argv[8] = cwd;
    argv[9] = "--no-expand";

    std::copy(flatArgs, flatArgs + argc, argv + ADDITIONAL_ARGS);
    argv[argc + ADDITIONAL_ARGS] = NULL;

    const char* const* envp = flatArgs + argc + 1;

    posix_spawnattr_t spawnAttributes;
    posix_spawnattr_init(&spawnAttributes);

    sched_param sched;
    sched.sched_priority = priority;
    posix_spawnattr_setschedparam(&spawnAttributes, &sched);

    pid_t newpid;
    int status = posix_spawn(&newpid, loaderPath.c_str(), NULL, NULL, (char**)argv, (char**)envp);

    posix_spawnattr_destroy(&spawnAttributes);
    delete[] argv;

    if (status != 0)
    {
        return -status;
    }

    loader_debugger_team_created(newpid);

    return newpid;
}
