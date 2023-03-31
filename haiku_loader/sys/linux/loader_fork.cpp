#include <cstddef>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/mman.h>
#include <unistd.h>

#include "haiku_area.h"
#include "haiku_errors.h"
#include "haiku_tls.h"
#include "loader_fork.h"
#include "loader_servercalls.h"
#include "loader_tls.h"
#include "loader_vchroot.h"
#include "servercalls.h"

extern bool loader_hserver_child_atfork();

// This function handles the forking of process states
// (runtime_loader, commpage, kernel connections,...)
int loader_fork()
{
    // Kernel connections are handled by pthread_atfork.
    int status = fork();
    if (status < 0)
    {
        return -errno;
    }
    if (status == 0)
    {
        // Set the thread_id slot.
        // libroot's getpid() function depends on it.
        tls_set(TLS_THREAD_ID_SLOT, (void*)(uintptr_t)gettid());
        loader_hserver_child_atfork();

        // Fix areas
        ssize_t cookie = 0;
        haiku_area_info areaInfo;
        uint32_t mapping;

        while (loader_hserver_call_get_next_area_info(0, &cookie, &areaInfo, &mapping) == B_OK)
        {
            if (mapping != REGION_PRIVATE_MAP)
            {
                continue;
            }

            if (!(areaInfo.protection & B_CLONEABLE_AREA))
            {
                continue;
            }

            std::filesystem::path path = std::filesystem::path(gHaikuPrefix) / HYCLONE_SHM_NAME / std::to_string(areaInfo.area);
            int fd = open(path.c_str(), O_RDWR);

            if (fd < 0)
            {
                continue;
            }

            int mmap_prot = 0;
            if (areaInfo.protection & B_READ_AREA)
            {
                mmap_prot |= PROT_READ;
            }
            if (areaInfo.protection & B_WRITE_AREA)
            {
                mmap_prot |= PROT_WRITE;
            }
            if (areaInfo.protection & B_EXECUTE_AREA)
            {
                mmap_prot |= PROT_EXEC;
            }

            mmap(areaInfo.address, areaInfo.size, mmap_prot, MAP_FIXED | MAP_SHARED, fd, 0);
            close(fd);
        }
    }
    return status;
}