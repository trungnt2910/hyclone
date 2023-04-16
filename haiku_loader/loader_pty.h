#ifndef __LOADER_PTY_H__
#define __LOADER_PTY_H__

#include <cstddef>

int loader_openpt(int openFlags);
int loader_grantpt(int masterFD);
int loader_ptsname(int masterFD, char* buffer, size_t bufferSize);
int loader_unlockpt(int masterFD);

#endif // __LOADER_PTY_H__