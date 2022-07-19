#ifndef __HAIKU_THREAD_H__
#define __HAIKU_THREAD_H__

#include "BeDefs.h"

/* Threads */

typedef enum
{
    B_THREAD_RUNNING        = 1,
    B_THREAD_READY,
    B_THREAD_RECEIVING,
    B_THREAD_ASLEEP,
    B_THREAD_SUSPENDED,
    B_THREAD_WAITING
} haiku_thread_state;

typedef struct
{
    thread_id           thread;
    team_id             team;
    char                name[B_OS_NAME_LENGTH];
    haiku_thread_state  state;
    int32               priority;
    sem_id              sem;
    bigtime_t           user_time;
    bigtime_t           kernel_time;
    void                *stack_base;
    void                *stack_end;
} haiku_thread_info;

#define B_IDLE_PRIORITY                 0
#define B_LOWEST_ACTIVE_PRIORITY        1
#define B_LOW_PRIORITY                  5
#define B_NORMAL_PRIORITY               10
#define B_DISPLAY_PRIORITY              15
#define B_URGENT_DISPLAY_PRIORITY       20
#define B_REAL_TIME_DISPLAY_PRIORITY    100
#define B_URGENT_PRIORITY               110
#define B_REAL_TIME_PRIORITY            120

#define B_SYSTEM_TIMEBASE               0
/* time base for snooze_*(), compatible with the clockid_t constants defined
   in <time.h> */

#define B_FIRST_REAL_TIME_PRIORITY      B_REAL_TIME_DISPLAY_PRIORITY

#endif // __HAIKU_THREAD_H__