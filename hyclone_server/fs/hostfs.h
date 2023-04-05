#ifndef __HYCLONE_HOSTFS_H__
#define __HYCLONE_HOSTFS_H__

#include "server_vfs.h"

// Generic base class for filesystems that uses files on the host.
// rootfs, systemfs, devfs and even the current "packagefs" should subclass this.
class HostfsDevice : public VfsDevice, private std::enable_shared_from_this<HostfsDevice>
{
private:
    static haiku_ino_t _Hash(uint64_t hostDev, uint64_t hostIno);
protected:
    std::filesystem::path _hostRoot;
    virtual bool _IsBlacklisted(const std::filesystem::path& path) const { return false; }
    virtual bool _IsBlacklisted(const std::filesystem::directory_entry& entry) const { return false; }
public:
    HostfsDevice(const std::filesystem::path& root, const std::filesystem::path& hostRoot);
    virtual ~HostfsDevice() override = default;

    virtual status_t GetPath(std::filesystem::path& path, bool& isSymlink) override;
    virtual status_t ReadStat(std::filesystem::path& path, haiku_stat& stat, bool& isSymlink) override;
    virtual status_t WriteStat(std::filesystem::path& path, const haiku_stat& stat,
        int statMask, bool& isSymlink) override;
    virtual status_t OpenDir(std::filesystem::path& path, VfsDir& dir, bool& isSymlink) override;
    virtual status_t ReadDir(VfsDir& dir, haiku_dirent& dirent) override;
    virtual status_t RewindDir(VfsDir& dir) override;

    const std::filesystem::path& GetHostRoot() const { return _hostRoot; }
};

#endif // __HYCLONE_HOSTFS_H__