#include <cstddef>
#include <cstdint>

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "BeDefs.h"
#include "errno_conversion.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_area.h"
#include "haiku_errors.h"
#include "linux_debug.h"
#include "linux_syscall.h"
#include "stringutils.h"

#ifdef B_HAIKU_64_BIT
const addr_t kMaxRandomize = 0x8000000000ul;
#else
const addr_t kMaxRandomize = 0x800000ul;
#endif

static int ProtBToLinux(int protection);

class MmanLock
{
public:
    MmanLock()
    {
        GET_HOSTCALLS()->lock_reserved_range_data();
    }

    ~MmanLock()
    {
        GET_HOSTCALLS()->unlock_reserved_range_data();
    }
};

extern "C"
{

int32_t MONIKA_EXPORT _kern_create_area(const char *name, void **address,
    uint32_t addressSpec, size_t size,
    uint32_t lock, uint32_t protection)
{
    if (lock == B_CONTIGUOUS || lock == B_LOMEM)
    {
        return HAIKU_POSIX_ENOSYS;
    }

    long result;

    void* hintAddr = *address;
    void* baseAddr = hintAddr;
    int mmap_flags = MAP_PRIVATE | MAP_ANON;
    int mmap_prot = 0;

    if (protection & B_READ_AREA)
        mmap_prot |= PROT_READ;
    if (protection & B_WRITE_AREA)
        mmap_prot |= PROT_WRITE;
    if (protection & B_EXECUTE_AREA)
        mmap_prot |= PROT_EXEC;
    if (protection & B_STACK_AREA)
    {
        mmap_flags |= MAP_STACK | MAP_GROWSDOWN;
        mmap_prot |= PROT_READ | PROT_WRITE | PROT_GROWSDOWN;
    }

    switch (addressSpec)
    {
        case B_ANY_ADDRESS:
        case B_RANDOMIZED_ANY_ADDRESS:
            hintAddr = NULL;
        break;
        case B_EXACT_ADDRESS:
            // B_EXACT_ADDRESS is NOT MAP_FIXED!
            // mmap_flags |= MAP_FIXED;
        break;
        case B_RANDOMIZED_BASE_ADDRESS:
        {
            addr_t randnum;
            result = LINUX_SYSCALL3(__NR_getrandom, &randnum, sizeof(addr_t), 0);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            randnum %= kMaxRandomize;
            hintAddr = (void*)((addr_t)hintAddr + randnum);
        }
        break;
    }

    MmanLock mmanLock;

    if (addressSpec == B_EXACT_ADDRESS && GET_HOSTCALLS()->is_in_reserved_range(hintAddr, size))
    {
        if (GET_HOSTCALLS()->reserved_range_longest_mappable_from(hintAddr, size) < size)
        {
            return HAIKU_POSIX_ENOMEM;
        }

        // Safe to call MAP_FIXED in this case.
        result = LINUX_SYSCALL6(__NR_mmap, hintAddr, size, mmap_prot, mmap_flags | MAP_FIXED, -1, 0);
        if (result < 0)
        {
            return LinuxToB(-result);
        }

        result = (long)hintAddr;

        GET_HOSTCALLS()->map_reserved_range(hintAddr, size);
    }
    else
    {
        result = LINUX_SYSCALL6(__NR_mmap, hintAddr, size, mmap_prot, mmap_flags, -1, 0);
        if (result < 0)
        {
            return LinuxToB(-result);
        }
    }


    switch (addressSpec)
    {
        case B_BASE_ADDRESS:
        case B_RANDOMIZED_BASE_ADDRESS:
            if (result < (long)baseAddr)
            {
                // Success, but not greater than base.
                LINUX_SYSCALL2(__NR_munmap, result, size);
                return HAIKU_POSIX_ENOMEM;
            }
        break;
        case B_EXACT_ADDRESS:
            if (result != (long)hintAddr)
            {
                // Success, but not at the address requested.
                LINUX_SYSCALL2(__NR_munmap, result, size);
                return HAIKU_POSIX_ENOMEM;
            }
    }

    hintAddr = (void *)result;

    switch (lock)
    {
        case B_NO_LOCK:
            break;
        case B_LAZY_LOCK:
            result = LINUX_SYSCALL3(__NR_mlock2, hintAddr, size, MLOCK_ONFAULT);
            break;
        case B_FULL_LOCK:
            result = LINUX_SYSCALL3(__NR_mlock2, hintAddr, size, 0);
            break;
        default:
            result = -ENOSYS;
    }

    if (result < 0)
    {
        LINUX_SYSCALL2(__NR_munmap, hintAddr, size);
        return LinuxToB(-result);
    }

    if (__gCommPageAddress == NULL)
    {
        trace("warning: commpage not initialized");
        return HAIKU_POSIX_ENOSYS;
    }

    struct haiku_area_info info;

    strlcpy(info.name, name, sizeof(info.name));
    info.size = size;
    info.lock = lock;
    info.protection = protection;
    info.address = hintAddr;
    info.ram_size = 0;
    info.copy_count = 0;
    info.in_count = 0;
    info.out_count = 0;

    int area_id = GET_SERVERCALLS()->register_area(&info, REGION_PRIVATE_MAP);

    if (area_id < 0)
    {
        LINUX_SYSCALL2(__NR_munmap, hintAddr, size);
        // This is actually an error code returned by the server.
        return area_id;
    }

    *address = hintAddr;

    if (protection & B_CLONEABLE_AREA)
    {
        char hostPath[PATH_MAX];
        if (GET_SERVERCALLS()->share_area(area_id, -1, 0, hostPath, sizeof(hostPath)) != B_OK)
        {
            return B_OK;
        }

        int open_flags = 0;
        if (protection & (B_READ_AREA | B_WRITE_AREA))
        {
            open_flags |= O_RDWR;
        }
        else if (protection & B_WRITE_AREA)
        {
            open_flags |= O_WRONLY;
        }
        else
        {
            open_flags |= O_RDONLY;
        }

        int fd = LINUX_SYSCALL3(__NR_open, (long)hostPath, open_flags, 0);
        if (fd < 0)
        {
            return B_OK;
        }

        mmap_flags |= MAP_SHARED | MAP_FIXED;
        mmap_flags &= ~(MAP_ANONYMOUS | MAP_PRIVATE);

        if (LINUX_SYSCALL6(__NR_mmap, hintAddr, size, mmap_prot, mmap_flags, fd, 0) == -1)
        {
            panic("WHY CAN'T I MAP_FIXED ON A FRESHLY MAPPED AREA???");
        }

        LINUX_SYSCALL1(__NR_close, fd);
    }

    return area_id;
}

area_id MONIKA_EXPORT _kern_clone_area(const char *name, void **address,
    uint32 addressSpec, uint32 protection, area_id sourceArea)
{
    struct haiku_area_info sourceInfo;
    long status = GET_SERVERCALLS()->get_area_info(sourceArea, &sourceInfo);

    if (status != B_OK)
    {
        return status;
    }

    if (!(sourceInfo.protection & B_CLONEABLE_AREA))
    {
        return B_BAD_VALUE;
    }

    void* hintAddr = *address;
    void* baseAddr = hintAddr;
    int mmap_flags = MAP_PRIVATE;
    int mmap_prot = 0;

    if (protection & B_READ_AREA)
        mmap_prot |= PROT_READ;
    if (protection & B_WRITE_AREA)
        mmap_prot |= PROT_WRITE;
    if (protection & B_EXECUTE_AREA)
        mmap_prot |= PROT_EXEC;
    if (protection & B_STACK_AREA)
    {
        mmap_flags |= MAP_STACK | MAP_GROWSDOWN;
        mmap_prot |= PROT_READ | PROT_WRITE | PROT_GROWSDOWN;
    }

    switch (addressSpec)
    {
        case B_ANY_ADDRESS:
        case B_RANDOMIZED_ANY_ADDRESS:
            hintAddr = NULL;
        break;
        case B_EXACT_ADDRESS:
            // B_EXACT_ADDRESS is NOT MAP_FIXED!
            // mmap_flags |= MAP_FIXED;
        break;
        case B_RANDOMIZED_BASE_ADDRESS:
        {
            addr_t randnum;
            status = LINUX_SYSCALL3(__NR_getrandom, &randnum, sizeof(addr_t), 0);
            if (status < 0)
            {
                return LinuxToB(-status);
            }
            randnum %= kMaxRandomize;
            hintAddr = (void*)((addr_t)hintAddr + randnum);
        }
        break;
        case B_CLONE_ADDRESS:
            hintAddr = sourceInfo.address;
        break;
    }
    
    char hostPath[PATH_MAX];
    status = GET_SERVERCALLS()->get_shared_area_path(sourceArea, hostPath, sizeof(hostPath));
    if (status != B_OK)
    {
        return status;
    }

    int open_flags = 0;
    if (protection & (B_READ_AREA | B_WRITE_AREA))
    {
        open_flags |= O_RDWR;
    }
    else if (protection & B_WRITE_AREA)
    {
        open_flags |= O_WRONLY;
    }
    else
    {
        open_flags |= O_RDONLY;
    }
    int fd = LINUX_SYSCALL3(__NR_open, hostPath, open_flags, 0);
    if (fd < 0)
    {
        return LinuxToB(-fd);
    }

    LINUX_SYSCALL1(__NR_fsync, fd);

    status = LINUX_SYSCALL6(__NR_mmap, hintAddr, sourceInfo.size, mmap_prot, mmap_flags, fd, 0);

    if (status == -1)
    {
        LINUX_SYSCALL1(__NR_close, fd);
        return LinuxToB(-status);
    }

    switch (addressSpec)
    {
        case B_BASE_ADDRESS:
        case B_RANDOMIZED_BASE_ADDRESS:
            if (status < (long)baseAddr)
            {
                // Success, but not greater than base.
                LINUX_SYSCALL2(__NR_munmap, status, sourceInfo.size);
                LINUX_SYSCALL1(__NR_close, fd);
                return B_NO_MEMORY;
            }
        break;
        case B_EXACT_ADDRESS:
            if (status != (long)hintAddr)
            {
                // Success, but not at the address requested.
                LINUX_SYSCALL2(__NR_munmap, status, sourceInfo.size);
                LINUX_SYSCALL1(__NR_close, fd);
                return B_NO_MEMORY;
            }
        case B_CLONE_ADDRESS:
            if (status != (long)sourceInfo.address)
            {
                // Success, but not at the address requested.
                LINUX_SYSCALL2(__NR_munmap, status, sourceInfo.size);
                LINUX_SYSCALL1(__NR_close, fd);
                return B_NO_MEMORY;
            }
    }

    hintAddr = (void *)status;

    struct haiku_area_info info;

    strlcpy(info.name, name, sizeof(info.name));
    info.size = sourceInfo.size;
    info.lock = sourceInfo.lock;
    info.protection = protection;
    info.address = hintAddr;
    info.ram_size = 0;
    info.copy_count = 0;
    info.in_count = 0;
    info.out_count = 0;

    int area_id = GET_SERVERCALLS()->register_area(&info, REGION_PRIVATE_MAP);

    if (area_id < 0)
    {
        LINUX_SYSCALL2(__NR_munmap, hintAddr, info.size);
        LINUX_SYSCALL1(__NR_close, fd);
        // This is actually an error code returned by the server.
        return area_id;
    }

    if (protection & B_CLONEABLE_AREA)
    {
        status = GET_SERVERCALLS()->share_area(area_id, fd, 0, hostPath, sizeof(hostPath));

        if (status != B_OK)
        {
            GET_SERVERCALLS()->unregister_area(area_id);
            LINUX_SYSCALL2(__NR_munmap, hintAddr, info.size);
            LINUX_SYSCALL1(__NR_close, fd);
            return status;
        }

        LINUX_SYSCALL1(__NR_close, fd);
        fd = LINUX_SYSCALL3(__NR_open, hostPath, open_flags, 0);
        if (fd < 0)
        {
            GET_SERVERCALLS()->unregister_area(area_id);
            LINUX_SYSCALL2(__NR_munmap, hintAddr, info.size);
            return LinuxToB(-fd);            
        }

        mmap_flags |= MAP_SHARED | MAP_FIXED;
        mmap_flags &= ~(MAP_ANONYMOUS | MAP_PRIVATE);

        if (LINUX_SYSCALL6(__NR_mmap, hintAddr, info.size, mmap_prot, mmap_flags, fd, 0) == -1)
        {
            panic("WHY CAN'T I MAP_FIXED ON A FRESHLY MAPPED AREA???");
        }
    }

    LINUX_SYSCALL1(__NR_close, fd);

    switch (sourceInfo.lock)
    {
        case B_NO_LOCK:
            break;
        case B_LAZY_LOCK:
            status = LINUX_SYSCALL3(__NR_mlock2, hintAddr, sourceInfo.size, MLOCK_ONFAULT);
            break;
        case B_FULL_LOCK:
            status = LINUX_SYSCALL3(__NR_mlock2, hintAddr, sourceInfo.size, 0);
            break;
        default:
            status = -ENOSYS;
    }

    if (status < 0)
    {
        GET_SERVERCALLS()->unregister_area(area_id);
        LINUX_SYSCALL2(__NR_munmap, hintAddr, info.size);
        return LinuxToB(-status);
    }

    *address = hintAddr;
    return area_id;
}

int MONIKA_EXPORT _kern_delete_area(int area)
{
    CHECK_COMMPAGE();

    struct haiku_area_info info;
    long status = GET_SERVERCALLS()->get_area_info(area, &info);

    if (status != B_OK)
    {
        return status;
    }

    MmanLock mmanLock;

    if (GET_HOSTCALLS()->is_in_reserved_range(info.address, info.size))
    {
        // Return the memory to the reserved ranges.
        status = LINUX_SYSCALL3(__NR_mprotect, info.address, info.size, PROT_NONE);
        GET_HOSTCALLS()->unmap_reserved_range(info.address, info.size);
    }
    else
    {
        status = LINUX_SYSCALL2(__NR_munmap, info.address, info.size);

        if (status < 0)
        {
            return LinuxToB(-status);
        }
    }

    if (status == B_OK)
    {
        GET_SERVERCALLS()->unregister_area(area);
    }

    return status;
}

int MONIKA_EXPORT _kern_set_area_protection(int area, uint32_t newProtection)
{
    CHECK_COMMPAGE();

    struct haiku_area_info info;
    long status = GET_SERVERCALLS()->get_area_info(area, &info);

    if (status != B_OK)
    {
        return status;
    }

    int mmap_prot = 0;
    if (newProtection & B_READ_AREA)
        mmap_prot |= PROT_READ;
    if (newProtection & B_WRITE_AREA)
        mmap_prot |= PROT_WRITE;
    if (newProtection & B_EXECUTE_AREA)
        mmap_prot |= PROT_EXEC;
    if (newProtection & B_STACK_AREA)
        mmap_prot |= PROT_READ | PROT_WRITE | PROT_GROWSDOWN;

    status = LINUX_SYSCALL3(__NR_mprotect, info.address, info.size, mmap_prot);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    status = GET_SERVERCALLS()->set_area_protection(area, newProtection);

    return status;
}

status_t MONIKA_EXPORT _kern_set_memory_protection(void *address, size_t size, uint32 protection)
{
    int mprotect_prot = 0;
    if (protection & B_READ_AREA)
        mprotect_prot |= PROT_READ;
    if (protection & B_WRITE_AREA)
        mprotect_prot |= PROT_WRITE;
    if (protection & B_EXECUTE_AREA)
        mprotect_prot |= PROT_EXEC;

    char* beginAddress = (char*)address;
    char* endAddress = beginAddress + size;

    char* currentAddress = beginAddress;

    if (size == 0)
    {
        return HAIKU_POSIX_EINVAL;
    }

    {
        MmanLock mmanLock;
        bool validMemoryRange = true;

        while (currentAddress < endAddress)
        {
            char* nextReservedAddress;
            size_t nextReservedAddressSize = GET_HOSTCALLS()->next_reserved_range(currentAddress, (void**)&nextReservedAddress);

            if (nextReservedAddressSize != 0)
            {
                if (nextReservedAddress >= endAddress)
                {
                    nextReservedAddress = endAddress;
                    nextReservedAddressSize = 0;
                }

                if (nextReservedAddress + nextReservedAddressSize > endAddress)
                {
                    nextReservedAddressSize = endAddress - nextReservedAddress;
                }
            }
            else
            {
                nextReservedAddress = endAddress;
                nextReservedAddressSize = 0;
            }

            // Ignore non-reserved addresses.
            if (currentAddress < nextReservedAddress)
            {
                currentAddress = nextReservedAddress;
            }

            // Checks all reserved addresses to see if they are mapped.
            if (nextReservedAddressSize != 0)
            {
                char* nextReservedAddressRangeEnd = nextReservedAddress + nextReservedAddressSize;
                while (currentAddress < nextReservedAddressRangeEnd)
                {
                    char* nextReservedRangeMappedAddress;
                    size_t nextReservedRangeMappedAddressSize = GET_HOSTCALLS()
                        ->next_reserved_range_mapping(currentAddress, (void**)&nextReservedRangeMappedAddress);

                    if (nextReservedRangeMappedAddressSize != 0)
                    {
                        if (nextReservedRangeMappedAddress >= nextReservedAddressRangeEnd)
                        {
                            nextReservedRangeMappedAddress = nextReservedAddressRangeEnd;
                            nextReservedRangeMappedAddressSize = 0;
                        }

                        if (nextReservedRangeMappedAddress + nextReservedRangeMappedAddressSize > nextReservedAddressRangeEnd)
                        {
                            nextReservedRangeMappedAddressSize = nextReservedAddressRangeEnd - nextReservedRangeMappedAddress;
                        }
                    }
                    else
                    {
                        nextReservedRangeMappedAddress = nextReservedAddressRangeEnd;
                        nextReservedRangeMappedAddressSize = 0;
                    }

                    // If the next mapped address is larger, the current address should collide
                    // with a reserved range, and that page hasn't been mapped.
                    if (nextReservedRangeMappedAddress > currentAddress)
                    {
                        validMemoryRange = false;
                        goto check_done;
                    }

                    currentAddress = nextReservedRangeMappedAddress + nextReservedRangeMappedAddressSize;
                }
            }
        }

    check_done:
        if (!validMemoryRange)
        {
            // Very weird, ENOMEM for a mprotect. But that's what Haiku does.
            return HAIKU_POSIX_ENOMEM;
        }
    }

    // Proceed as normal.
    long status = GET_SERVERCALLS()->set_memory_protection(address, size, protection);

    if (status != B_OK)
    {
        return status;
    }

    status = LINUX_SYSCALL3(__NR_mprotect, address, size, mprotect_prot);

    return LinuxToB(-status);
}

// This function is undocumented, but here's the behavior
// according to some tests on a Haiku machine:
// Reserves a range of memory that hasn't been mapped
// into any area.
//
// create_area will avoid mapping into this range of memory,
// unless B_EXACT_ADDRESS is specified.
// If B_EXACT_ADDRESS is specified, if the requested range of memory
// collides with a reserved region in any way, the call will fail,
// unless the requested range of memory is completely contained in a
// reserved region.
//
// reserve_address_range will fail if the area is already mapped
// or if the range is already reserved.
//
// Any resize_area attempts will fail if it grows into a reserved range.
// A resize_area attempt will succeed if the base is from a reserved ranged,
// and it grows in the range of the reserved range. If it grows outside of the reserved
// range, the behavior may vary.
//
// unreserve_address_range does not unmap any area.
int MONIKA_EXPORT _kern_reserve_address_range(void **address, uint32_t addressSpec, size_t size)
{
    // The function requires hostcalls.
    CHECK_COMMPAGE();

    long result;
    void* hintAddr = *address;
    void* baseAddr = hintAddr;
    int mmap_flags = MAP_PRIVATE | MAP_ANON;
    int mmap_prot = PROT_NONE;

    switch (addressSpec)
    {
        case B_ANY_KERNEL_ADDRESS:
            return B_BAD_VALUE;
        case B_ANY_ADDRESS:
        case B_RANDOMIZED_ANY_ADDRESS:
            hintAddr = NULL;
        break;
        case B_EXACT_ADDRESS:
            mmap_flags |= MAP_FIXED;
        break;
        case B_RANDOMIZED_BASE_ADDRESS:
        {
            addr_t randnum;
            result = LINUX_SYSCALL3(__NR_getrandom, &randnum, sizeof(addr_t), 0);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            randnum %= kMaxRandomize;
            hintAddr = (void*)((addr_t)hintAddr + randnum);
        }
        break;
    }

    // A mmap would do the job.
    result = LINUX_SYSCALL6(__NR_mmap, hintAddr, size, mmap_prot, mmap_flags, -1, 0);
    if (result < 0)
    {
        return LinuxToB(-result);
    }

    switch (addressSpec)
    {
        case B_BASE_ADDRESS:
        case B_RANDOMIZED_BASE_ADDRESS:
            if (result < (long)baseAddr)
            {
                // Success, but not greater than base.
                LINUX_SYSCALL2(__NR_munmap, result, size);
                return HAIKU_POSIX_ENOMEM;
            }
        break;
        case B_EXACT_ADDRESS:
            if (result != (long)hintAddr)
            {
                // Success, but not at the address requested.
                LINUX_SYSCALL2(__NR_munmap, result, size);
                return HAIKU_POSIX_ENOMEM;
            }
    }

    *address = (void *)result;

    MmanLock mmanLock;

    GET_HOSTCALLS()->register_reserved_range(*address, size);

    return B_OK;
}

int MONIKA_EXPORT _kern_unreserve_address_range(void* address, size_t size)
{
    // The function requires hostcalls.
    CHECK_COMMPAGE();

    MmanLock mmanLock;

    if (!GET_HOSTCALLS()->is_in_reserved_range(address, size))
    {
        return B_BAD_VALUE;
    }

    long result = LINUX_SYSCALL2(__NR_munmap, address, size);
    if (result < 0)
    {
        return LinuxToB(-result);
    }

    GET_HOSTCALLS()->unregister_reserved_range(address, size);

    return B_OK;
}

int MONIKA_EXPORT _kern_unmap_memory(void *address, size_t size)
{
    MmanLock mmanLock;

    char* beginAddress = (char*)address;
    char* endAddress = beginAddress + size;

    char* currentAddress = beginAddress;

    if (size == 0)
    {
        return HAIKU_POSIX_EINVAL;
    }

    // Do the actual unmapping.
    while (currentAddress < endAddress)
    {
        char* nextReservedAddress;
        size_t nextReservedAddressSize = GET_HOSTCALLS()->next_reserved_range(currentAddress, (void**)&nextReservedAddress);

        if (nextReservedAddressSize != 0)
        {
            if (nextReservedAddress >= endAddress)
            {
                nextReservedAddress = endAddress;
                nextReservedAddressSize = 0;
            }

            if (nextReservedAddress + nextReservedAddressSize > endAddress)
            {
                nextReservedAddressSize = endAddress - nextReservedAddress;
            }
        }
        else
        {
            nextReservedAddress = endAddress;
            nextReservedAddressSize = 0;
        }

        // Unmap non-reserved addresses.
        if (currentAddress < nextReservedAddress)
        {
            LINUX_SYSCALL2(__NR_munmap, currentAddress, nextReservedAddress - currentAddress);
            currentAddress = nextReservedAddress;
        }

        // Ignore reserved addresses before currentAddress
        if (nextReservedAddress < currentAddress)
        {
            nextReservedAddressSize -= currentAddress - nextReservedAddress;
            nextReservedAddress = currentAddress;
        }

        // Return reserved addresses.
        if (nextReservedAddressSize != 0)
        {
            LINUX_SYSCALL3(__NR_mprotect, nextReservedAddress, nextReservedAddressSize, PROT_NONE);
            GET_HOSTCALLS()->unmap_reserved_range(nextReservedAddress, nextReservedAddressSize);
            currentAddress = nextReservedAddress + nextReservedAddressSize;
        }
    }

    currentAddress = beginAddress;

    // Better do this in the server, where all area info is kept.
    GET_SERVERCALLS()->unmap_memory(currentAddress, size);

    return B_OK;
}

int MONIKA_EXPORT _kern_map_file(const char *name, void **address,
	uint32_t addressSpec, size_t size, uint32_t protection,
	uint32_t mapping, bool unmapAddressRange, int fd,
	off_t offset)
{
    void* hintAddr = *address;
    int mmap_flags = (fd < 0) ? MAP_ANON : 0;
    int mmap_prot = 0;
    int open_flags = 0;
    long result;

    if (protection & B_READ_AREA)
        mmap_prot |= PROT_READ;
    if (protection & B_WRITE_AREA)
        mmap_prot |= PROT_WRITE;
    if (protection & B_EXECUTE_AREA)
        mmap_prot |= PROT_EXEC;
    if (protection & B_STACK_AREA)
    {
        mmap_flags |= MAP_STACK | MAP_GROWSDOWN;
        mmap_prot |= PROT_READ | PROT_WRITE | PROT_GROWSDOWN;
    }
    if (protection & B_CLONEABLE_AREA)
    {
        if (mapping == REGION_PRIVATE_MAP)
        {
            // This is a weird case. To properly handle this we'll
            // have to use hooks at fork.
            return B_BAD_VALUE;
        }
        mmap_flags |= MAP_SHARED;
    }

    if (protection & (B_READ_AREA | B_WRITE_AREA))
    {
        open_flags |= O_RDWR;
    }
    else if (protection & B_EXECUTE_AREA)
    {
        open_flags |= O_RDONLY;
    }
    else
    {
        open_flags |= O_RDONLY;
    }

    switch (addressSpec)
    {
        case B_ANY_ADDRESS:
        case B_RANDOMIZED_ANY_ADDRESS:
            hintAddr = NULL;
        break;
        case B_EXACT_ADDRESS:
            mmap_flags |= MAP_FIXED;
        break;
    }

    switch (mapping)
    {
        case REGION_NO_PRIVATE_MAP:
            mmap_flags |= MAP_SHARED;
        break;
        case REGION_PRIVATE_MAP:
            mmap_flags |= MAP_PRIVATE;
        break;
    }

    MmanLock mmanLock;

    // These two variables are only valid when they are needed:
    // when unmapAddressRange is true, or when addressSpec is B_EXACT_ADDRESS.
    bool collidesWithReservedRange = false;
    bool isInReservedRange = false;

    if ((unmapAddressRange || addressSpec == B_EXACT_ADDRESS))
    {
        collidesWithReservedRange = GET_HOSTCALLS()->collides_with_reserved_range(hintAddr, size);
        isInReservedRange = GET_HOSTCALLS()->is_in_reserved_range(hintAddr, size);

        if (collidesWithReservedRange && !isInReservedRange)
        {
            // The address range is not reserved, but it collides with a reserved range.
            // We can't unmap the address range, neither can we map it to a new file.
            return B_BAD_VALUE;
        }
    }

    if (unmapAddressRange)
    {
        // Attempt to unmap the address range first.

        // _kern_unmap_memory also acquires a lock.
        GET_HOSTCALLS()->unlock_reserved_range_data();
        result = _kern_unmap_memory(*address, size);
        GET_HOSTCALLS()->lock_reserved_range_data();
        if (result != B_OK)
        {
            return result;
        }
    }

    if (addressSpec == B_EXACT_ADDRESS && isInReservedRange)
    {
        if (GET_HOSTCALLS()->reserved_range_longest_mappable_from(hintAddr, size) < size)
        {
            return B_BAD_VALUE;
        }

        result = LINUX_SYSCALL2(__NR_munmap, hintAddr, size);
        if (result < 0)
        {
            return LinuxToB(-result);
        }

        result = LINUX_SYSCALL6(__NR_mmap, hintAddr, size, mmap_prot, mmap_flags, fd, offset);
        if (result < 0)
        {
            return LinuxToB(-result);
        }

        GET_HOSTCALLS()->map_reserved_range(hintAddr, size);
    }
    // As for B_EXACT_ADDRESS and the mapped area collides with an existing
    // reserved area, this scenario cannot happen, as the code above has
    // already handled this case.
    else
    {
        result = LINUX_SYSCALL6(__NR_mmap, hintAddr, size, mmap_prot, mmap_flags, fd, offset);
        if (result < 0)
        {
            return LinuxToB(-result);
        }
    }

    switch (addressSpec)
    {
        case B_BASE_ADDRESS:
        case B_RANDOMIZED_BASE_ADDRESS:
            if (result < (long)hintAddr)
            {
                // Success, but not greater than base.
                LINUX_SYSCALL2(__NR_munmap, result, size);
                return HAIKU_POSIX_ENOMEM;
            }
        break;
        case B_EXACT_ADDRESS:
            if (result != (long)hintAddr)
            {
                // Success, but not at the address requested.
                LINUX_SYSCALL2(__NR_munmap, result, size);
                return HAIKU_POSIX_ENOMEM;
            }
    }

    hintAddr = (void *)result;

    struct haiku_area_info info;

    strlcpy(info.name, name, sizeof(info.name));
    info.size = size;
    info.lock = 0;
    info.protection = protection;
    info.address = hintAddr;
    info.ram_size = 0;
    info.copy_count = 0;
    info.in_count = 0;
    info.out_count = 0;

    // TODO: What happens when B_EXACT_ADDRESS is specified and
    // the mapped area collides with an existing one?
    int area_id = GET_SERVERCALLS()->register_area(&info, mapping);

    if (area_id < 0)
    {
        LINUX_SYSCALL2(__NR_munmap, hintAddr, size);
        return area_id;
    }

    *address = hintAddr;

    if (protection & B_CLONEABLE_AREA || mapping == REGION_NO_PRIVATE_MAP)
    {
        char hostPath[PATH_MAX];
        if (GET_SERVERCALLS()->share_area(area_id, fd, offset, hostPath, sizeof(hostPath)) != B_OK)
        {
            // If we return an error state at this point things will be highly inconsistent.
            return B_OK;
        }

        fd = LINUX_SYSCALL3(__NR_open, (long)hostPath, open_flags, 0);
        if (fd < 0)
        {
            return B_OK;
        }

        mmap_flags |= MAP_SHARED | MAP_FIXED;
        mmap_flags &= ~(MAP_ANONYMOUS | MAP_PRIVATE);

        if (LINUX_SYSCALL6(__NR_mmap, hintAddr, size, mmap_prot, mmap_flags | MAP_FIXED, fd, offset) == -1)
        {
            // This is probably not Linux but a poorly implemented emulator.
            panic("WHY CAN'T I MAP_FIXED ON A FRESHLY MAPPED AREA???");
        }

        LINUX_SYSCALL1(__NR_close, fd);
    }

    return area_id;
}

int MONIKA_EXPORT _kern_resize_area(int32_t area, size_t newSize)
{
    struct haiku_area_info info;
    long status = GET_SERVERCALLS()->get_area_info(area, &info);

    if (status != B_OK)
    {
        return status;
    }

    MmanLock mmanLock;

    if (newSize == info.size)
    {
        return B_OK;
    }

    status = GET_SERVERCALLS()->resize_area(area, newSize);
    if (status != B_OK)
    {
        return status;
    }

    if (newSize < info.size)
    {
        if (GET_HOSTCALLS()->is_in_reserved_range(info.address, info.size))
        {
            status = LINUX_SYSCALL6(__NR_mmap, (uint8_t*)info.address + newSize, info.size - newSize, PROT_NONE,
                MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

            if (status < 0)
            {
                GET_SERVERCALLS()->resize_area(area, info.size);
                return LinuxToB(-status);
            }

            GET_HOSTCALLS()->unmap_reserved_range((uint8_t*)info.address + newSize, info.size - newSize);
        }
        else
        {
            // Quite impossible for this area to be in a reserved range.
            // Reserved ranges cannot be created memory where there's already
            // an area. Furthermore, areas cannot grow into reserved ranges.
            status = LINUX_SYSCALL2(__NR_munmap, (uint8_t*)info.address + newSize, info.size - newSize);

            if (status < 0)
            {
                GET_SERVERCALLS()->resize_area(area, info.size);
                return LinuxToB(-status);
            }
        }
    }
    else // if (newSize > info.size)
    {
        bool isInReservedRange = GET_HOSTCALLS()->is_in_reserved_range(info.address, info.size);
        size_t mappableSize;
        if (isInReservedRange)
        {
            mappableSize =
                GET_HOSTCALLS()->reserved_range_longest_mappable_from((uint8_t*)info.address + info.size, newSize - info.size);

            status = LINUX_SYSCALL2(__NR_munmap, (uint8_t*)info.address + info.size, mappableSize);
            GET_HOSTCALLS()->map_reserved_range((uint8_t*)info.address + info.size, mappableSize);

            if (status < 0)
            {
                GET_SERVERCALLS()->resize_area(area, info.size);
                return LinuxToB(-status);
            }
        }
        status = LINUX_SYSCALL3(__NR_mremap, info.address, info.size, newSize);

        if (status < 0)
        {
            if (isInReservedRange)
            {
                LINUX_SYSCALL6(__NR_mmap, (uint8_t*)info.address + info.size, mappableSize, PROT_NONE,
                    MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                GET_HOSTCALLS()->unmap_reserved_range((uint8_t*)info.address + info.size, newSize - info.size);
            }
            GET_SERVERCALLS()->resize_area(area, info.size);
            return LinuxToB(-status);
        }
    }

    return B_OK;
}

area_id MONIKA_EXPORT _kern_area_for(void *address)
{
    return GET_SERVERCALLS()->area_for(address);
}

status_t MONIKA_EXPORT _kern_get_area_info(area_id area, void* info)
{
    return GET_SERVERCALLS()->get_area_info(area, info);
}

status_t MONIKA_EXPORT _kern_get_next_area_info(team_id team, ssize_t *cookie, void* info)
{
    return GET_SERVERCALLS()->get_next_area_info(team, cookie, info, NULL);
}

status_t MONIKA_EXPORT _kern_mlock(const void* address, size_t size)
{
    long status = LINUX_SYSCALL2(__NR_mlock, address, size);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    status = GET_SERVERCALLS()->set_memory_lock((void*)address, size, B_FULL_LOCK);

    if (status != B_OK)
    {
        LINUX_SYSCALL2(__NR_munlock, address, size);
        return status;
    }

    return B_OK;
}

}

int ProtBToLinux(int protection)
{
    int mmap_prot = 0;
    if (protection & B_READ_AREA)
        mmap_prot |= PROT_READ;
    if (protection & B_WRITE_AREA)
        mmap_prot |= PROT_WRITE;
    if (protection & B_EXECUTE_AREA)
        mmap_prot |= PROT_EXEC;
    if (protection & B_STACK_AREA)
        mmap_prot |= PROT_READ | PROT_WRITE | PROT_GROWSDOWN;

    return mmap_prot;
}