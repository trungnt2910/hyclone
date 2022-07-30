#include <cstddef>

#include "BeDefs.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "linux_debug.h"
#include "linux_syscall.h"

extern "C"
{

void MONIKA_EXPORT _kern_image_relocated(image_id id)
{
    if (__gCommPageAddress != NULL)
    {
        GET_SERVERCALLS()->image_relocated(id);
    }
}

status_t MONIKA_EXPORT _kern_get_image_info(image_id id, void *info, size_t size)
{
    CHECK_COMMPAGE();
    return GET_SERVERCALLS()->get_image_info(id, info, size);
}

status_t MONIKA_EXPORT _kern_get_next_image_info(team_id team, int32* cookie, void *info, size_t size)
{
    CHECK_COMMPAGE();
    return GET_SERVERCALLS()->get_next_image_info(team, cookie, info, size);
}

status_t MONIKA_EXPORT _kern_register_image(void* info, size_t size)
{
    CHECK_COMMPAGE();

    return GET_SERVERCALLS()->register_image(info, size);
}

status_t MONIKA_EXPORT _kern_unregister_image(image_id id)
{
    CHECK_COMMPAGE();

    return GET_SERVERCALLS()->unregister_image(id);
}

void MONIKA_EXPORT _kern_loading_app_failed(status_t error)
{
	if (error >= B_OK)
        error = B_ERROR;

    trace("_kern_loading_app_failed");

    if (__gCommPageAddress != NULL)
    {
        GET_HOSTCALLS()->at_exit(error);
    }

    LINUX_SYSCALL1(__NR_exit, error);
}

}