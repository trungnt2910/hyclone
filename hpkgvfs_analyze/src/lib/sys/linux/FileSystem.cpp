#include <chrono>
#include <filesystem>
#include <vector>

// POSIX
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Linux
#include <sys/xattr.h>

#include <hpkgvfs/Entry.h>

namespace HpkgVfs::Platform
{
    extern void SetDateModifed(const std::filesystem::path& path, const std::filesystem::file_time_type& time)
    {
        auto systemTime = std::chrono::file_clock::to_sys(time).time_since_epoch();
        struct timespec tv[2];
        tv[1].tv_sec = std::chrono::duration_cast<std::chrono::seconds>(systemTime).count();
        tv[1].tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(systemTime).count();
        tv[0].tv_nsec = UTIME_OMIT;
        utimensat(AT_FDCWD, path.c_str(), tv, AT_SYMLINK_NOFOLLOW);
    }

    extern void SetDateAccess(const std::filesystem::path& path, const std::filesystem::file_time_type& time)
    {
        auto systemTime = std::chrono::file_clock::to_sys(time).time_since_epoch();
        struct timespec tv[2];
        tv[0].tv_sec = std::chrono::duration_cast<std::chrono::seconds>(systemTime).count();
        tv[0].tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(systemTime).count();
        tv[1].tv_nsec = UTIME_OMIT;
        utimensat(AT_FDCWD, path.c_str(), tv, AT_SYMLINK_NOFOLLOW);
    }

    extern void SetDateCreate(const std::filesystem::path& path, const std::filesystem::file_time_type& time)
    {
        // Impossible on Linux.
    }

    extern void SetOwner(const std::filesystem::path& path, const std::string& user, const std::string& group)
    {
        uid_t uid = -1;
        gid_t gid = -1;

        if (user != "")
        {
            passwd* pwd = getpwnam(user.c_str());
            if (pwd != nullptr)
            {
                uid = pwd->pw_uid;
            }
        }

        if (group != "")
        {
            struct group* grp = getgrnam(group.c_str());
            if (grp != nullptr)
            {
                gid = grp->gr_gid;
            }
        }

        lchown(path.c_str(), uid, gid);
    }

    extern void WriteExtendedAttributes(const std::filesystem::path& path, const std::vector<ExtendedAttribute>& attributes)
    {
        for (const auto& a : attributes)
        {
            lsetxattr(path.c_str(), a.Name.c_str(), a.Data.data(), a.Data.size(), 0);
        }
    }
}