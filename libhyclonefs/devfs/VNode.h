#ifndef __LIBHYCLONEFS_DEVFS_VNODE_H__
#define __LIBHYCLONEFS_DEVFS_VNODE_H__

#include "Volume.h"

#include "fssh_defs.h"
#include "fssh_dirent.h"
#include "fssh_errors.h"
#include "fssh_fs_interface.h"
#include "fssh_stat.h"

#include <new>

class VNode
{
    public:
        VNode(Volume* volume, fssh_ino_t id = -1, fssh_ino_t parentID = -1);
        virtual ~VNode() = default;

        fssh_ino_t GetID() const { return fID; }
        const struct fssh_stat* GetStatData() const { return &fStatData; }
        struct fssh_stat* GetStatData() { return &fStatData; }

        bool IsDir() const { return FSSH_S_ISDIR(GetStatData()->fssh_st_mode); }

        virtual fssh_ino_t GetParentID() const { return fParentID; }
        virtual fssh_dev_t GetParentDevice() const { return fVolume->GetID(); }
        virtual fssh_status_t ReadDir(int* index, void* buffer, fssh_size_t bufferSize, uint32_t* _num) { return FSSH_B_NOT_A_DIRECTORY; }
        virtual fssh_status_t Lookup(const char* name, fssh_vnode_id* vnid) { return FSSH_B_NOT_A_DIRECTORY; }
        virtual fssh_status_t Open(int openMode, void** cookie) { return FSSH_B_NOT_ALLOWED; }
        virtual fssh_status_t Read(void* cookie, fssh_off_t pos, void* buffer, fssh_size_t* _length) { return FSSH_B_NOT_ALLOWED; }
        virtual fssh_status_t Write(void* cookie, fssh_off_t pos, const void* buffer, fssh_size_t* _length) { return FSSH_B_NOT_ALLOWED; }
        virtual fssh_status_t Close() { return FSSH_B_OK; }
        virtual fssh_status_t FreeCookie(void* cookie) { return FSSH_B_OK; }

    protected:
        mutable fssh_ino_t fParentID = -1;
        fssh_ino_t fID;
        Volume* fVolume;
        struct fssh_stat fStatData;
};

#endif // __LIBHYCLONEFS_DEVFS_VNODE_H__