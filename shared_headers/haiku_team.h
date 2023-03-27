#ifndef __HAIKU_TEAM_H__
#define __HAIKU_TEAM_H__

#include "BeDefs.h"

/* Teams */

typedef struct
{
    team_id team;
    int32 thread_count;
    int32 image_count;
    int32 area_count;
    thread_id debugger_nub_thread;
    port_id debugger_nub_port;
    int32 argc;
    char args[64];
    uid_t uid;
    gid_t gid;
} haiku_team_info;

#define B_CURRENT_TEAM	0
#define B_SYSTEM_TEAM	1

#endif // __HAIKU_TEAM_H__