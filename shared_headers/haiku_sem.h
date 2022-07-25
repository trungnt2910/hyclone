#ifndef __HAIKU_SEM_H__
#define __HAIKU_SEM_H__

#include "BeDefs.h"

/* Semaphores */

typedef struct haiku_sem_info
{
    sem_id      sem;
    team_id     team;
    char        name[B_OS_NAME_LENGTH];
    int32       count;
    thread_id   latest_holder;
} haiku_sem_info;

/* semaphore flags */
enum
{
    B_CAN_INTERRUPT             = 0x01,      /* acquisition of the semaphore can be
                                                interrupted (system use only) */
    B_CHECK_PERMISSION          = 0x04,      /* ownership will be checked (system use
                                                only) */
    B_KILL_CAN_INTERRUPT        = 0x20,      /* acquisition of the semaphore can be
                                                interrupted by SIGKILL[THR], even
                                                if not B_CAN_INTERRUPT (system use
                                                only) */

    /* release_sem_etc() only flags */
    B_DO_NOT_RESCHEDULE         = 0x02,      /* thread is not rescheduled */
    B_RELEASE_ALL               = 0x08,      /* all waiting threads will be woken up,
                                                count will be zeroed */
    B_RELEASE_IF_WAITING_ONLY   = 0x10       /* release count only if there are any
                                                threads waiting */
};

#endif // __HAIKU_SEM_H__