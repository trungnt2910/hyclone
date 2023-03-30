#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <string.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "entry_ref.h"
#include "servercalls.h"
#include "server_memory.h"
#include "server_prefix.h"

bool server_setup_memory()
{
    auto shmpath = std::filesystem::path(gHaikuPrefix) / HYCLONE_SHM_NAME;

    umount(shmpath.c_str());

    std::filesystem::create_directories(shmpath);

    if (mount("shm", shmpath.c_str(), "tmpfs", 0, "size=100%") != 0)
    {
        std::cerr << "Failed to mount shmfs: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

intptr_t server_open_shared_file(const char* name, size_t size, bool writable)
{
    auto path = std::filesystem::path(gHaikuPrefix) / HYCLONE_SHM_NAME / name;
    int fd = writable ?
        open(path.c_str(), O_RDWR | O_CREAT | O_EXCL) :
        open(path.c_str(), O_RDONLY | O_CREAT | O_EXCL);
    if (fd < 0)
    {
        return -1;
    }
    if (writable && ftruncate(fd, size) != 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}

intptr_t server_open_file(const char* name, bool writable)
{
    int fd = writable ?
        open(name, O_RDWR) :
        open(name, O_RDONLY);
    if (fd < 0)
    {
        return -1;
    }
    return fd;
}

bool server_resize_shared_file(intptr_t handle, size_t size)
{
    return ftruncate((int)handle, size) == 0;
}

void server_close_file(intptr_t handle)
{
    close((int)handle);
}

intptr_t server_acquire_process_file_handle(int pid, intptr_t handle, bool writable)
{
    auto path = std::filesystem::path("proc") / std::to_string(pid) / "fd" / std::to_string((int)handle);
    int fd = open(path.c_str(), writable ? O_RDWR : O_RDONLY);
    if (fd < 0)
    {
        return -1;
    }
    return fd;
}

bool server_get_entry_ref(intptr_t handle, EntryRef& ref)
{
    struct stat st;
    if (fstat((int)handle, &st) != 0)
    {
        return false;
    }
    ref = EntryRef(st.st_dev, st.st_ino);
    return true;
}

void* server_map_memory(intptr_t handle, size_t size, size_t offset, bool writable)
{
    int prot = writable ? (PROT_READ | PROT_WRITE) : PROT_READ;
    void* result = mmap((void*)handle, size, prot, MAP_SHARED, (int)handle, offset);
    return result;
}

void* server_resize_memory(void* address, size_t size, size_t newSize)
{
    return mremap(address, size, newSize, MREMAP_MAYMOVE);
}

void server_unmap_memory(void* address, size_t size)
{
    munmap(address, size);
}

size_t server_get_page_size()
{
    static size_t pageSize = sysconf(_SC_PAGESIZE);
    return pageSize;
}