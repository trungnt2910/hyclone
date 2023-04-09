/*
 * Copyright 2002-2011, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_FS_ATTR_H__
#define __HAIKU_FS_ATTR_H__

#include "BeDefs.h"

typedef struct haiku_attr_info
{
    uint32 type;
    haiku_off_t size;
} haiku_attr_info;

#endif /* __HAIKU_FS_ATTR_H__ */