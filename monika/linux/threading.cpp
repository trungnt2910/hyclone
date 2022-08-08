#include <cstring>

#include "BeDefs.h"
#include "errno_conversion.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "haiku_thread.h"
#include "linux_debug.h"

extern "C"
{

thread_id MONIKA_EXPORT _kern_spawn_thread(void* attributes)
{
    long status = GET_HOSTCALLS()->spawn_thread(attributes);

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return status;
}

status_t MONIKA_EXPORT _kern_suspend_thread(thread_id thread)
{
    return GET_SERVERCALLS()->suspend_thread(thread);
}
status_t MONIKA_EXPORT _kern_resume_thread(thread_id thread)
{
    return GET_SERVERCALLS()->resume_thread(thread);
}

status_t MONIKA_EXPORT _kern_exit_thread(status_t returnValue)
{
    GET_HOSTCALLS()->exit_thread(returnValue);
    return 0;
}

status_t MONIKA_EXPORT _kern_wait_for_thread(thread_id thread, status_t *_returnCode)
{
    long status = GET_HOSTCALLS()->wait_for_thread(thread, _returnCode);
    if (status < 0)
    {
        return LinuxToB(-status);
    }
    return 0;
}

void MONIKA_EXPORT _kern_find_thread()
{
    panic("_kern_find_thread not implemented");
}

status_t MONIKA_EXPORT _kern_get_thread_info(thread_id id, haiku_thread_info *info)
{
    return GET_SERVERCALLS()->get_thread_info(id, info);
}

void MONIKA_EXPORT _kern_mutex_lock()
{
    panic("_kern_mutex_lock not implemented");
}

void MONIKA_EXPORT _kern_mutex_unlock()
{
    panic("_kern_mutex_unlock not implemented");
}

sem_id MONIKA_EXPORT _kern_create_sem(int count, const char *name)
{
    return GET_SERVERCALLS()->create_sem(count, name, (name == NULL) ? 0 : strlen(name));
}

status_t MONIKA_EXPORT _kern_delete_sem(sem_id id)
{
    return GET_SERVERCALLS()->delete_sem(id);
}

}