#ifndef __LIBHYCLONEFS_DEVFS_RANDOMVNODE_H__
#define __LIBHYCLONEFS_DEVFS_RANDOMVNODE_H__

#include "VNode.h"

#include <random>

class RandomVNode : public VNode
{
    public:
        RandomVNode(Volume* volume, fssh_ino_t parentID);
        virtual ~RandomVNode() = default;
        virtual fssh_status_t Open(int openMode, void** cookie) override;
        virtual fssh_status_t Read(void* cookie, fssh_off_t pos, void* buffer, fssh_size_t* _length) override;
        virtual fssh_status_t Write(void* cookie, fssh_off_t pos, const void* buffer, fssh_size_t* _length) override;
        virtual fssh_status_t Close() override;
        virtual fssh_status_t FreeCookie(void* cookie) override;

    private:
        std::random_device fRandomDevice;
};

#endif // __LIBHYCLONEFS_DEVFS_RANDOMVNODE_H__