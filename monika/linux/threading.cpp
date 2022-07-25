#include <cstring>

#include "BeDefs.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "haiku_thread.h"
#include "linux_debug.h"

extern "C"
{

void MONIKA_EXPORT _kern_find_thread()
{
    panic("_kern_find_thread not implemented");
}

status_t MONIKA_EXPORT _kern_get_thread_info(thread_id id, haiku_thread_info *info)
{
    trace("stub: _kern_get_thread_info");
    return B_OK;
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
    return GET_SERVERCALLS()->create_sem(count, name, strlen(name));
}

}