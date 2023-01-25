#ifndef __LIBHYCLONEFS_DEVFS_STATDATA_H__
#define __LIBHYCLONEFS_DEVFS_STATDATA_H__
 
#include "fssh_defs.h"

#include <iostream>
 
class StatData
{
    public:
        fssh_ino_t GetID() const { return 0; }
};
 
 #endif // __LIBHYCLONEFS_DEVFS_STATDATA_H__