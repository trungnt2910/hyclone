#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_set>

#include "BeDefs.h"
#include "haiku_errors.h"
#include "haiku_fcntl.h"
#include "loader_debugger.h"
#include "loader_elf.h"
#include "loader_exec.h"
#include "loader_servercalls.h"
#include "loader_vchroot.h"

const unsigned char s_ElfMagic[] = { 0x7f, 'E', 'L', 'F' };

int (*loader_haiku_test_executable)(const char* name, char* invoker) = NULL;

static bool IsHaikuExecutable(Elf64_Ehdr_* ehdr, size_t size)
{
    int i, n;
    bool res = false;
    const char *stab;
    const Elf64_Sym_* st;
    if ((stab = GetElfStringTable(ehdr, size)) &&
        (st = GetElfSymbolTable(ehdr, size, &n)))
    {
        for (i = 0; i < n; ++i)
        {
            if (ELF64_ST_TYPE_(st[i].info) == STT_OBJECT_ &&
                !strcmp(stab + Read32(st[i].name), "_gSharedObjectHaikuVersion"))
            {
                res = true;
                break;
            }
        }
    }
    return res;
}

int loader_exec(const char* path, const char* const* flatArgs, size_t flatArgsSize, int argc, int envc, int umask)
{
    if (path == NULL)
    {
        return B_BAD_ADDRESS;
    }

    char hostPath[PATH_MAX];
    loader_hserver_call_vchroot_expandat(HAIKU_AT_FDCWD, path, strlen(path), true, hostPath, sizeof(hostPath));

    // Despite having a hook of Haiku's test_executable function,
    // we still have to conduct our own check as the function
    // mistakes native Linux executables for Haiku executables.
    bool isHaikuExecutable = false;
    struct stat statbuf;
    if (stat(hostPath, &statbuf) != -1)
    {
        int fd = open(hostPath, O_RDONLY);
        if (fd != -1)
        {
            void *tmp = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
            if (tmp != MAP_FAILED)
            {
                if (Read32(tmp) == Read32(s_ElfMagic))
                {
                    isHaikuExecutable = IsHaikuExecutable((Elf64_Ehdr_*)tmp, statbuf.st_size);
                }
                else
                {
                    isHaikuExecutable = false;
                }
                munmap(tmp, statbuf.st_size);
            }
        }
    }

    int status;

    if (isHaikuExecutable)
    {
        loader_hserver_call_exec(true);

        const int additionalArgs = 12;

        const char** argv = new const char*[argc + additionalArgs + 1];
        std::error_code ec;
        std::string loaderPath = std::filesystem::canonical("/proc/self/exe", ec);
        if (ec)
        {
            loaderPath = "/proc/self/exe";
        }
        const std::string umaskValue = std::to_string(umask);
        const std::string debuggerInfo = loader_debugger_serialize_info();

        char cwd[PATH_MAX];
        loader_hserver_call_getcwd(cwd, sizeof(cwd), false);

        char root[PATH_MAX];
        loader_hserver_call_get_root(root, sizeof(root));

        argv[0] = loaderPath.c_str();
        argv[1] = "--umask";
        argv[2] = umaskValue.c_str();
        argv[3] = "--debugger";
        argv[4] = debuggerInfo.c_str();
        argv[5] = "--prefix";
        argv[6] = gHaikuPrefix.c_str();
        argv[7] = "--cwd";
        argv[8] = cwd;
        argv[9] = "--root";
        argv[10] = root;
        argv[11] = "--no-expand";

        std::copy(flatArgs, flatArgs + argc, argv + additionalArgs);
        argv[argc + additionalArgs] = NULL;

        const char* const* envp = flatArgs + argc + 1;

        status = execve(loaderPath.c_str(), (char* const*)argv, (char* const*)envp);

        delete[] argv;
    }
    else
    {
        const char** argv;

        if (flatArgs[0] && flatArgs[0][0] == '/')
        {
            argv = new const char*[argc + 1];
            std::copy(flatArgs, flatArgs + argc, argv);
            argv[argc] = NULL;

            char* hostArgv0 = new char[PATH_MAX];
            loader_hserver_call_vchroot_expandat(HAIKU_AT_FDCWD, flatArgs[0], strlen(flatArgs[0]), true, hostArgv0, PATH_MAX);

            argv[0] = hostArgv0;
        }
        else
        {
            argv = (const char **)flatArgs;
        }

        int envc = 0;
        while (flatArgs[argc + 1 + envc])
        {
            ++envc;
        }

        const char** envp = new const char*[envc + 1];
        std::copy(flatArgs + argc + 1, flatArgs + argc + 1 + envc, envp);
        envp[envc] = NULL;

        std::string hostPathEnv = "PATH=";

        for (int i = 0; i < envc; ++i)
        {
            if (strncmp(envp[i], "PATH=", 5) == 0)
            {
                const char* hPath = getenv("HPATH");
                std::unordered_set<std::string> hPaths;
                std::string currentPath;

                if (hPath != NULL)
                {
                    std::stringstream hPathStream(hPath);

                    while (std::getline(hPathStream, currentPath, ':'))
                    {
                        hPaths.insert(currentPath);
                    }
                }

                std::stringstream ss(envp[i] + 5);
                char currentHostPath[PATH_MAX];

                while (std::getline(ss, currentPath, ':'))
                {
                    if (hPaths.contains(currentPath))
                    {
                        continue;
                    }

                    loader_hserver_call_vchroot_expandat(HAIKU_AT_FDCWD, currentPath.c_str(), currentPath.length(), true,
                        currentHostPath, sizeof(currentHostPath));

                    if (hostPathEnv.length() > 0)
                    {
                        hostPathEnv += ":";
                    }

                    hostPathEnv += currentHostPath;
                }

                envp[i] = hostPathEnv.c_str();
            }
        }

        status = execve(hostPath, (char* const*)argv, (char* const*)envp);

        if (flatArgs[0] && flatArgs[0][0] == '/')
        {
            delete[] argv[0];
            delete[] argv;
        }

        delete[] envp;
    }

    loader_hserver_call_exec(false);

    if (status == -1)
    {
        return -errno;
    }

    std::cerr << "loader_exec: execve returned without an error status!" << std::endl;
    return 0;
}

int loader_test_executable(const char* name, char* invoker)
{
    if (invoker != NULL)
    {
        invoker[0] = '\0';
    }

    status_t status = B_OK;
    if (loader_haiku_test_executable)
    {
        status = loader_haiku_test_executable(name, invoker);
    }

    if (status == B_NOT_AN_EXECUTABLE)
    {
        // This probably means that the file is a script of some kind.
        return status;
    }

    // For binaries that Haiku doesn't like (especially PE),
    // Linux will handle them.
    return B_OK;
}