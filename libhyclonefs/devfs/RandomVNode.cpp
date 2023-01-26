#include "RandomVNode.h"

#include "fssh_fcntl.h"
#include "fssh_string.h"

#include <chrono>
#include <iostream>

RandomVNode::RandomVNode(Volume* volume, fssh_ino_t parentID)
    : VNode(volume, -1, parentID)
{
    fID = (fssh_ino_t)(std::hash<std::string>()(std::to_string(parentID) + "/random") >> 1);

    fStatData.fssh_st_dev = volume->GetID();
    fStatData.fssh_st_ino = fID;
    fStatData.fssh_st_mode = FSSH_S_IFCHR | 0666;
    fStatData.fssh_st_nlink = 1;
    fStatData.fssh_st_uid = 0;
    fStatData.fssh_st_gid = 0;
    fStatData.fssh_st_size = 0;
    fStatData.fssh_st_blksize = FSSH_B_PAGE_SIZE;
    auto now = std::chrono::system_clock::now();
    auto tv_sec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    auto tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() % 1000000000;
    fStatData.fssh_st_atim.tv_sec = tv_sec;
    fStatData.fssh_st_atim.tv_nsec = tv_nsec;
    fStatData.fssh_st_mtim.tv_sec = tv_sec;
    fStatData.fssh_st_mtim.tv_nsec = tv_nsec;
    fStatData.fssh_st_ctim.tv_sec = tv_sec;
    fStatData.fssh_st_ctim.tv_nsec = tv_nsec;
    fStatData.fssh_st_type = 0;
    fStatData.fssh_st_blocks = 0;
}

fssh_status_t RandomVNode::Open(int openMode, void** cookie)
{
    if (openMode & FSSH_O_TRUNC)
    {
        return FSSH_B_UNSUPPORTED;
    }

    if (openMode & FSSH_O_DIRECTORY)
    {
        return FSSH_B_NOT_A_DIRECTORY;
    }

    int* openModePtr = new(std::nothrow) int(openMode);
    if (openModePtr == nullptr)
    {
        return FSSH_B_NO_MEMORY;
    }

    *cookie = openModePtr;

    return FSSH_B_OK;
}

fssh_status_t RandomVNode::Read(void* cookie, fssh_off_t pos, void* buffer, fssh_size_t* bufferSize)
{
    int openMode = *(int*)cookie;

    if (openMode & FSSH_O_WRONLY)
    {
        return FSSH_B_NOT_ALLOWED;
    }

    if (*bufferSize == 0)
    {
        return 0;
    }

    fssh_size_t readSize = 0;

    char* bufferStart = (char*)buffer;
    const fssh_size_t resultAlign = alignof(std::random_device::result_type);
    char* alignedBufferStart = (char*)((((fssh_addr_t)bufferStart + resultAlign - 1) / resultAlign) * resultAlign);
    std::random_device::result_type firstResult = fRandomDevice();

    fssh_memcpy(bufferStart, &firstResult, alignedBufferStart - bufferStart);
    readSize += alignedBufferStart - bufferStart;

    char* bufferEnd = bufferStart + *bufferSize;
    char* alignedBufferEnd = (char*)(((fssh_addr_t)bufferEnd / resultAlign) * resultAlign);

    while (alignedBufferStart < alignedBufferEnd)
    {
        ((std::random_device::result_type*)alignedBufferStart)[0] = fRandomDevice();
        alignedBufferStart += sizeof(std::random_device::result_type);
        readSize += sizeof(std::random_device::result_type);
    }

    firstResult = fRandomDevice();
    fssh_memcpy(alignedBufferEnd, &firstResult, bufferEnd - alignedBufferEnd);
    readSize += bufferEnd - alignedBufferEnd;

    *bufferSize = readSize;

    return FSSH_B_OK;
}

fssh_status_t RandomVNode::Close()
{
    return FSSH_B_OK;
}

fssh_status_t RandomVNode::FreeCookie(void* cookie)
{
    delete (int*)cookie;

    return FSSH_B_OK;
}

fssh_status_t RandomVNode::Write(void* cookie, fssh_off_t pos, const void* buffer, fssh_size_t* bufferSize)
{
    int openMode = *(int*)cookie;

    if (openMode & FSSH_O_RDONLY)
    {
        return FSSH_B_NOT_ALLOWED;
    }

    // On normal Unix systems, writing to /dev/random mixes more entropy into the pool.
    // However, we don't have a pool, so we just ignore the writes.

    return FSSH_B_OK;
}