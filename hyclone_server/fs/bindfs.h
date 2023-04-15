#ifndef __HYCLONE_BINDFS_H__
#define __HYCLONE_BINDFS_H__

#include "server_vfs.h"

class BindfsDevice : public VfsDevice
{
private:
    static haiku_ino_t _Hash(uint64_t hostDev, uint64_t hostIno);

    std::filesystem::path _boundRoot;

    status_t _TransformPath(std::filesystem::path& path);
public:
    BindfsDevice(const std::filesystem::path& root, const std::filesystem::path& boundRoot,
        uint32 mountFlags = 0);

    virtual status_t GetPath(std::filesystem::path& path, bool& isSymlink) override;
    virtual status_t GetAttrPath(std::filesystem::path& path, const std::string& name,
        uint32 type, bool createNew, bool& isSymlink) override;
    virtual status_t RealPath(std::filesystem::path& path, bool& isSymlink) override;
    virtual status_t ReadStat(std::filesystem::path& path, haiku_stat& stat, bool& isSymlink) override;
    virtual status_t WriteStat(std::filesystem::path& path, const haiku_stat& stat,
        int statMask, bool& isSymlink) override;
    virtual status_t StatAttr(const std::filesystem::path& path, const std::string& name,
        haiku_attr_info& info) override;
    virtual status_t TransformDirent(const std::filesystem::path& path, haiku_dirent& dirent) override;
    virtual haiku_ssize_t ReadAttr(const std::filesystem::path& path, const std::string& name, size_t pos,
        void* buffer, size_t size) override;
    virtual haiku_ssize_t WriteAttr(const std::filesystem::path& path, const std::string& name, uint32 type,
        size_t pos, const void* buffer, size_t size) override;
    virtual haiku_ssize_t RemoveAttr(const std::filesystem::path& path, const std::string& name)
        override;

    static status_t Mount(const std::filesystem::path& path,
        const std::filesystem::path& device, uint32 flags,
        const std::string& args, std::shared_ptr<VfsDevice>& output);
};

#endif // __HYCLONE_BINDFS_H__