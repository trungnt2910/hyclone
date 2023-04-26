#include <cassert>
#include <mutex>
#include <sys/inotify.h>
#include <thread>
#include <unordered_map>

#include "BeDefs.h"
#include "entry_ref.h"
#include "haiku_errors.h"
#include "server_errno.h"
#include "server_native.h"
#include "server_nodemonitor.h"
#include "server_vfs.h"
#include "system.h"

static int sInotifyFd = -1;
static std::once_flag sInotifyInitFlag;
static status_t sInotifyStatus = B_NO_INIT;
static std::thread sInotifyThread;
static std::mutex sInotifyMutex;
static std::unordered_map<int, std::unordered_set<EntryRef>> sInotifyListeners;
static std::unordered_map<EntryRef, int> sInotifyWatches;

static void server_init_inotify();
static void server_inotify_thread_main();

status_t server_add_native_monitor(const std::filesystem::path& hostPath, haiku_dev_t device, haiku_ino_t node)
{
    std::call_once(sInotifyInitFlag, server_init_inotify);

    if (sInotifyStatus != B_OK)
    {
        return sInotifyStatus;
    }

    int wd = inotify_add_watch(sInotifyFd, hostPath.c_str(), IN_DELETE | IN_CREATE | IN_MOVED_FROM | IN_MOVED_TO);
    if (wd < 0)
    {
        return LinuxToB(errno);
    }

    {
        auto lock = std::unique_lock(sInotifyMutex);
        assert(!sInotifyListeners[wd].contains(EntryRef(device, node)));
        sInotifyListeners[wd].insert(EntryRef(device, node));
        sInotifyWatches[EntryRef(device, node)] = wd;
    }

    return B_OK;
}

status_t server_remove_native_monitor(haiku_dev_t device, haiku_ino_t node)
{
    if (sInotifyStatus != B_OK)
    {
        return sInotifyStatus;
    }

    {
        auto lock = std::unique_lock(sInotifyMutex);
        if (!sInotifyWatches.contains(EntryRef(device, node)))
        {
            return B_BAD_VALUE;
        }
        int wd = sInotifyWatches[EntryRef(device, node)];
        sInotifyWatches.erase(EntryRef(device, node));
        sInotifyListeners[wd].erase(EntryRef(device, node));
        if (sInotifyListeners[wd].empty())
        {
            inotify_rm_watch(sInotifyFd, wd);
            sInotifyListeners.erase(wd);
        }
    }

    return B_OK;
}

void server_init_inotify()
{
    sInotifyFd = inotify_init();
    if (sInotifyFd < 0)
    {
        sInotifyStatus = LinuxToB(errno);
    }

    sInotifyThread = std::thread(server_inotify_thread_main);
    sInotifyStatus = B_OK;
}

void server_inotify_thread_main()
{
    uint8_t buffer[std::max((size_t)4096, sizeof(inotify_event) + NAME_MAX + 1)];
    auto& system = System::GetInstance();
    auto& nodeMonitorService = system.GetNodeMonitorService();
    auto& vfsService = system.GetVfsService();
    while (!system.IsShuttingDown())
    {
        int length = read(sInotifyFd, buffer, sizeof(buffer));
        if (length < 0)
        {
            sInotifyStatus = LinuxToB(errno);
            break;
        }

        for (uint8_t* ptr = buffer; ptr < buffer + length;)
        {
            const inotify_event& event = *(inotify_event*)ptr;
            ptr += sizeof(inotify_event) + sizeof(inotify_event) + event.len;

            std::vector<EntryRef> refs;

            {
                auto lock = std::unique_lock(sInotifyMutex);
                if (!sInotifyListeners.contains(event.wd))
                {
                    continue;
                }
                refs.insert(refs.end(), sInotifyListeners[event.wd].begin(), sInotifyListeners[event.wd].end());
            }

            if (event.mask & (IN_DELETE | IN_CREATE | IN_MOVED_FROM | IN_MOVED_TO))
            {
                std::vector<haiku_ino_t> nodes;
                nodes.resize(refs.size());

                {
                    auto lock = vfsService.Lock();
                    std::string path;

                    status_t status;

                    for (size_t i = 0; i < refs.size(); i++)
                    {
                        nodes[i] = (haiku_ino_t)-1;
                        if (!vfsService.GetEntryRef(refs[i], path))
                        {
                            continue;
                        }

                        auto nodePath = (std::filesystem::path(path) / event.name).lexically_normal();

                        if (event.mask & (IN_CREATE | IN_MOVED_TO))
                        {
                            haiku_stat stat;
                            status = vfsService.ReadStat(nodePath, stat, false);

                            if (status != B_OK)
                            {
                                continue;
                            }

                            nodes[i] = stat.st_ino;

                            vfsService.RegisterEntryRef(EntryRef(stat.st_dev, stat.st_ino), nodePath);
                        }
                        else // if (event.mask & (IN_DELETE | IN_MOVED_FROM))
                        {
                            // We cannot read the stat of the node that was deleted or moved.
                            // However, we can check the VFS service cache for any registered
                            // entry refs for that path.
                            EntryRef ref;
                            if (!vfsService.SearchEntryRef(nodePath, ref))
                            {
                                continue;
                            }

                            nodes[i] = ref.GetInode();

                            vfsService.UnregisterEntryRef(ref);
                        }
                    }
                }

                for (size_t i = 0; i < refs.size(); ++i)
                {
                    if (nodes[i] == (haiku_ino_t)-1)
                    {
                        continue;
                    }

                    // We're translating IN_MOVED_FROM and IN_MOVED_TO to B_ENTRY_REMOVED and
                    // B_ENTRY_CREATED, respectively, instead of B_ENTRY_MOVED because:
                    // - B_ENTRY_MOVED requires information about both the old and new paths,
                    // but Linux does not provide a way to get both.
                    // - It is not guaranteed that paths involved in IN_MOVED_FROM and IN_MOVED_TO
                    // will be on the same emulated device.

                    if (event.mask & (IN_DELETE | IN_MOVED_FROM))
                    {
                        nodeMonitorService.NotifyEntryCreatedOrRemoved(B_ENTRY_REMOVED,
                            refs[i].GetDevice(), refs[i].GetInode(), event.name, nodes[i]);
                    }

                    if (event.mask & (IN_CREATE | IN_MOVED_TO))
                    {
                        nodeMonitorService.NotifyEntryCreatedOrRemoved(B_ENTRY_CREATED,
                            refs[i].GetDevice(), refs[i].GetInode(), event.name, nodes[i]);
                    }
                }
            }
        }
    }
}