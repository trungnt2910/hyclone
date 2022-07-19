#ifndef __REALPATH_H__
#define __REALPATH_H__

#include <limits.h>

long realpath(const char *path, char resolved[PATH_MAX], bool resolveSymlinks);

#endif // __REALPATH_H__