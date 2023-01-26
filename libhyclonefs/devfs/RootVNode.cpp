#include "devfs.h"
#include "RandomVNode.h"
#include "RootVNode.h"
#include "Volume.h"

#include "fssh_defs.h"
#include "fssh_os.h"
#include "fssh_string.h"
#include "vfs.h"

#include <chrono>
#include <iostream>
#include <new>

const int kRootVNodeID = 1;

RootVNode::RootVNode(Volume* volume)
    : VNode(volume, kRootVNodeID)
{
    fID = kRootVNodeID;
    fssh_memset(&fStatData, 0, sizeof(fStatData));
    fStatData.fssh_st_dev = volume->GetID();
    fStatData.fssh_st_ino = fID;
    fStatData.fssh_st_mode = FSSH_S_IFDIR | 0755;
    fStatData.fssh_st_nlink = 1;
    fStatData.fssh_st_uid = 0;
    fStatData.fssh_st_gid = 0;
    fStatData.fssh_st_size = FSSH_B_PAGE_SIZE;
    fStatData.fssh_st_blksize = FSSH_B_PAGE_SIZE;
    fStatData.fssh_st_atim.tv_sec = 0;
    fStatData.fssh_st_atim.tv_nsec = 0;
    fStatData.fssh_st_mtim.tv_sec = 0;
    fStatData.fssh_st_mtim.tv_nsec = 0;
    auto now = std::chrono::system_clock::now();
    fStatData.fssh_st_ctim.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    fStatData.fssh_st_crtim.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() % 1000000000;
    fStatData.fssh_st_type = 0;
    fStatData.fssh_st_blocks = 1;
}

RootVNode::~RootVNode()
{
    if (fChildrenPopulated)
    {
        for (auto child : fChildren)
        {
            FSShell::vfs_put_vnode(child.second);
        }
    }
}

void RootVNode::_PopulateChildren() const
{
    if (fChildrenPopulated)
    {
        return;
    }

    fssh_status_t error = FSSH_B_OK;

    if (error == FSSH_B_OK)
    {
        RandomVNode* randomVNode = new (std::nothrow) RandomVNode(fVolume, fID);
        if (randomVNode == nullptr)
        {
            error = FSSH_B_NO_MEMORY;
        }
        else
        {
            error = fssh_publish_vnode(fVolume->GetFSVolume(), randomVNode->GetID(), randomVNode, &gDevFSVNodeOps, FSSH_S_IFCHR, 0);
            if (error == FSSH_B_OK)
            {
                fssh_fs_vnode* randomFSVNode;
                error = FSShell::vfs_get_vnode(fVolume->GetFSVolume()->id, randomVNode->GetID(), (void**)&randomFSVNode);
                if (error == FSSH_B_OK)
                {
                    fChildren["random"] = randomFSVNode;
                }
            }
        }
    }

    fChildrenPopulated = true;
    return;
}

fssh_ino_t RootVNode::GetParentID() const
{
    if (fParentID == -1)
    {
        fssh_status_t error;

        struct fssh_stat stat;
        error = FSShell::vfs_stat_vnode(fVolume->GetMountPoint(), &stat);

        if (error == FSSH_B_OK)
        {
            fParentID = stat.fssh_st_ino;
        }
    }

    return fParentID;
}

fssh_dev_t RootVNode::GetParentDevice() const
{
    if (fParentDevice == -1)
    {
        fssh_status_t error;

        struct fssh_stat stat;
        error = FSShell::vfs_stat_vnode(fVolume->GetMountPoint(), &stat);

        if (error == FSSH_B_OK)
        {
            fParentDevice = stat.fssh_st_dev;
        }
    }

    return fParentDevice;
}

fssh_status_t RootVNode::ReadDir(int* index, void* buffer, fssh_size_t bufferSize, uint32_t* _num)
{
    uint32_t maxCount = *_num;

    if (maxCount == 0)
    {
        return FSSH_B_OK;
    }

    *_num = 0;

    fssh_status_t error = FSSH_B_OK;

    char* bufferStart = (char*)buffer;
    char* bufferEnd = bufferStart + bufferSize;

    while (bufferStart < bufferEnd && *_num < maxCount)
    {
        if (*index == -2)
        {
            const char name[] = "..";
            if (bufferSize < sizeof(fssh_dirent) + sizeof(name))
            {
                break;
            }

            fssh_dirent* dirent = (fssh_dirent*)bufferStart;
            dirent->d_dev = GetParentDevice();
            dirent->d_pdev = 0;
            dirent->d_ino = GetParentID();
            dirent->d_pino = 0;
            dirent->d_reclen = sizeof(fssh_dirent) + sizeof(name);
            fssh_strcpy(dirent->d_name, name);

            bufferStart += dirent->d_reclen;
            bufferSize -= dirent->d_reclen;
        }
        else if (*index == -1)
        {
            const char name[] = ".";
            if (bufferSize < sizeof(fssh_dirent) + sizeof(name))
            {
                break;
            }

            fssh_dirent* dirent = (fssh_dirent*)bufferStart;
            dirent->d_dev = fVolume->GetID();
            dirent->d_pdev = GetParentDevice();
            dirent->d_ino = fID;
            dirent->d_pino = GetParentID();
            dirent->d_reclen = sizeof(fssh_dirent) + sizeof(name);
            fssh_strcpy(dirent->d_name, name);

            bufferStart += dirent->d_reclen;
            bufferSize -= dirent->d_reclen;
        }
        else
        {
            _PopulateChildren();

            if (*index >= (int)fChildren.size())
            {
                break;
            }

            auto it = fChildren.begin();
            std::advance(it, *index);

            const std::string& name = it->first;
            fssh_fs_vnode* child = it->second;

            if (bufferSize < sizeof(fssh_dirent) + name.size() + 1)
            {
                break;
            }

            fssh_dirent* dirent = (fssh_dirent*)bufferStart;
            dirent->d_dev = fVolume->GetID();
            dirent->d_pdev = fVolume->GetID();
            dirent->d_ino = ((VNode*)child->private_node)->GetID();
            dirent->d_pino = fID;
            dirent->d_reclen = sizeof(fssh_dirent) + name.size() + 1;
            fssh_strcpy(dirent->d_name, name.c_str());

            bufferStart += dirent->d_reclen;
            bufferSize -= dirent->d_reclen;
        }

        ++(*index);
        ++(*_num);
    }

    if (error == FSSH_B_OK && *_num == 0 && *index < fChildren.size())
    {
        error = FSSH_B_BUFFER_OVERFLOW;
    }

    return error;
}

fssh_status_t RootVNode::Lookup(const char* name, fssh_ino_t* _id)
{
    _PopulateChildren();

    auto it = fChildren.find(name);
    if (it == fChildren.end())
    {
        return FSSH_B_ENTRY_NOT_FOUND;
    }

    *_id = ((VNode*)it->second->private_node)->GetID();
    return FSSH_B_OK;
}