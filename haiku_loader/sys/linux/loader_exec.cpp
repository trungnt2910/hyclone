#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>
#include "loader_exec.h"
#include "loader_servercalls.h"

int loader_exec(const char* path, const char* const* flatArgs, size_t flatArgsSize, int argc, int envc, int umask)
{
    const char** argv = new const char*[argc + 4];
    const std::string loaderPath = std::filesystem::canonical("/proc/self/exe").string();
    const std::string umaskValue = std::to_string(umask);

    argv[0] = loaderPath.c_str();
    argv[1] = "--umask";
    argv[2] = umaskValue.c_str();

    std::copy(flatArgs, flatArgs + argc, argv + 3);
    argv[argc + 3] = NULL;

    const char* const* envp = flatArgs + argc + 1;

    loader_hserver_call_disconnect();

    // std::cerr << "execve: " << path << std::endl;

    int status = execve(loaderPath.c_str(), (char* const*)argv, (char* const*)envp);

    if (status == -1)
    {
        return -errno;
    }

    std::cerr << "loader_exec: execve returned without an error status!" << std::endl;

    return 0;
}