#ifndef __LIBHYCLONEFS_DEVFS_ROOTVNODE_H__
#define __LIBHYCLONEFS_DEVFS_ROOTVNODE_H__

#include "VNode.h"

#include <map>
#include <string>

class Volume;

class RootVNode : public VNode
{
    public:
        RootVNode(Volume* volume);
        virtual ~RootVNode() override; 

        virtual fssh_status_t ReadDir(int* index, void* buffer, fssh_size_t bufferSize, uint32_t* _num) override;
        virtual fssh_ino_t GetParentID() const override;
        virtual fssh_dev_t GetParentDevice() const override;
        virtual fssh_status_t Lookup(const char* name, fssh_vnode_id* vnid) override;

    protected:
        mutable fssh_dev_t fParentDevice = -1;
        mutable std::map<std::string, fssh_fs_vnode*> fChildren;

    private:
        mutable bool fChildrenPopulated = false;
        void _PopulateChildren() const;
};

#endif // __LIBHYCLONEFS_DEVFS_ROOTVNODE_H__