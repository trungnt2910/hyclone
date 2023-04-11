#include <cstddef>

#include "BeDefs.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "linux_debug.h"
#include "linux_syscall.h"

extern "C"
{

void _moni_image_relocated(image_id id)
{
    if (__gCommPageAddress != NULL)
    {
        GET_SERVERCALLS()->image_relocated(id);
    }
}

status_t _moni_get_image_info(image_id id, void *info, size_t size)
{
    CHECK_COMMPAGE();
    return GET_SERVERCALLS()->get_image_info(id, info, size);
}

status_t _moni_get_next_image_info(team_id team, int32* cookie, void *info, size_t size)
{
    CHECK_COMMPAGE();
    return GET_SERVERCALLS()->get_next_image_info(team, cookie, info, size);
}

status_t _moni_register_image(void* info, size_t size)
{
    CHECK_COMMPAGE();

    return GET_SERVERCALLS()->register_image(info, size);
}

status_t _moni_unregister_image(image_id id)
{
    CHECK_COMMPAGE();

    return GET_SERVERCALLS()->unregister_image(id);
}

void _moni_loading_app_failed(status_t error)
{
	if (error >= B_OK)
        error = B_ERROR;

    trace("_kern_loading_app_failed");

    GET_SERVERCALLS()->notify_loading_app(error);

    if (__gCommPageAddress != NULL)
    {
        GET_HOSTCALLS()->at_exit(error);
    }

    LINUX_SYSCALL1(__NR_exit, error);
}

}