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

#include "loader_elf.h"
#include "loader_exec.h"
#include "loader_servercalls.h"
#include "loader_vchroot.h"

const unsigned char s_ElfMagic[] = { 0x7f, 'E', 'L', 'F' };

static bool IsHaikuExecutable(Elf64_Ehdr_ *ehdr, size_t size) {
  int i, n;
  bool res = false;
  const char *stab;
  const Elf64_Sym_ *st;
  if ((stab = GetElfStringTable(ehdr, size)) &&
      (st = GetElfSymbolTable(ehdr, size, &n))) {
    for (i = 0; i < n; ++i) {
      if (ELF64_ST_TYPE_(st[i].info) == STT_OBJECT_ &&
          !strcmp(stab + Read32(st[i].name), "_gSharedObjectHaikuVersion")) {
        res = true;
        break;
      }
    }
  }
  return res;
}

int loader_exec(const char* path, const char* const* flatArgs, size_t flatArgsSize, int argc, int envc, int umask)
{
    char hostPath[PATH_MAX];
    loader_vchroot_expand(path, hostPath, sizeof(hostPath));

    struct stat statbuf;
    bool isHaikuExecutable = false;
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
                    isHaikuExecutable = IsHaikuExecutable((Elf64_Ehdr_ *)tmp, statbuf.st_size);
                }
                else
                {
                    isHaikuExecutable = false;
                }
                munmap(tmp, statbuf.st_size);
            }
        }
    }

    loader_hserver_call_disconnect();

    int status;

    if (isHaikuExecutable)
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

        status = execve(loaderPath.c_str(), (char* const*)argv, (char* const*)envp);
    }
    else
    {
        const char* const* envp = flatArgs + argc + 1;
        status = execve(hostPath, (char* const*)flatArgs, (char* const*)envp);
    }

    if (status == -1)
    {
        return -errno;
    }

    std::cerr << "loader_exec: execve returned without an error status!" << std::endl;
    return 0;
}