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

typedef struct haiku_port_message_info {
    size_t      size;
    haiku_uid_t sender;
    haiku_gid_t sender_group;
    team_id     sender_team;
} haiku_port_message_info;

#endif