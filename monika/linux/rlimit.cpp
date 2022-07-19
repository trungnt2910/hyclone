#include <cstddef>
#include <sys/resource.h>

#include "BeDefs.h"
#include "errno_conversion.h"
#include "export.h"
#include "haiku_errors.h"
#include "linux_debug.h"
#include "linux_syscall.h"

/* getrlimit()/setrlimit() definitions */

typedef addr_t haiku_rlim_t;

struct haiku_rlimit
{
    haiku_rlim_t rlim_cur; /* current soft limit */
    haiku_rlim_t rlim_max; /* hard limit */
};

/* ToDo: the only supported mode is RLIMIT_NOFILE right now */
#define HAIKU_RLIMIT_CORE     0   /* size of the core file */
#define HAIKU_RLIMIT_CPU      1   /* CPU time per team */
#define HAIKU_RLIMIT_DATA     2   /* data segment size */
#define HAIKU_RLIMIT_FSIZE    3   /* file size */
#define HAIKU_RLIMIT_NOFILE   4   /* number of open files */
#define HAIKU_RLIMIT_STACK    5   /* stack size */
#define HAIKU_RLIMIT_AS       6   /* address space size */
/* Haiku-specifics */
#define HAIKU_RLIMIT_NOVMON   7   /* number of open vnode monitors */

#define HAIKU_RLIM_NLIMITS    8   /* number of resource limits */

#define HAIKU_RLIM_INFINITY   (0xffffffffUL)
#define HAIKU_RLIM_SAVED_MAX  RLIM_INFINITY
#define HAIKU_RLIM_SAVED_CUR  RLIM_INFINITY

int RlimitBToLinux(int rlimit);

extern "C"
{

int MONIKA_EXPORT _kern_getrlimit(int resource, struct haiku_rlimit *rlp)
{
    switch (resource)
    {
#define SUPPORTED_RLIMIT(name) \
    case HAIKU_##name: break;
#define UNSUPPORTED_RLIMIT(name) \
    case HAIKU_##name: trace("Unsupported rlimit: "#name); return HAIKU_POSIX_ENOSYS;
#include "rlimit_values.h"
#undef SUPPORTED_RLIMIT
#undef UNSUPPORTED_RLIMIT
        default:
            return HAIKU_POSIX_EINVAL;
    }

    struct rlimit rl;
    long status = LINUX_SYSCALL2(__NR_getrlimit, RlimitBToLinux(resource), &rl);
    if (status < 0)
    {
        return LinuxToB(-status);
    }

    rlp->rlim_cur = (haiku_rlim_t)rl.rlim_cur;
    rlp->rlim_max = (haiku_rlim_t)rl.rlim_max;

    return B_OK;
}

}

int RlimitBToLinux(int rlimit)
{
    switch (rlimit)
    {
#define SUPPORTED_RLIMIT(name) \
    case HAIKU_##name: return name;
#include "rlimit_values.h"
#undef SUPPORTED_RLIMIT
    }

    return -1;
}