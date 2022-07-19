#ifndef __HAIKU_PORT_H__
#define __HAIKU_PORT_H__

#include "BeDefs.h"

typedef struct haiku_port_info
{
    port_id     port;
    team_id     team;
    char        name[B_OS_NAME_LENGTH];
    int32       capacity;    /* queue depth */
    int32       queue_count; /* # msgs waiting to be read */
    int32       total_count; /* total # msgs read so far */
} haiku_port_info;

#endif