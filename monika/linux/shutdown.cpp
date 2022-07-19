#include "BeDefs.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "linux_syscall.h"
#include "servercalls.h"

extern "C"
{

status_t MONIKA_EXPORT _kern_shutdown(bool reboot)
{
    if (__gCommPageAddress != NULL)
    {
        return GET_SERVERCALLS()->shutdown(reboot);
    }

    return HAIKU_POSIX_ENOSYS;
}

}