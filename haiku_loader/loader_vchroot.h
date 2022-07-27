#ifndef __LOADER_VCHROOT_H__
#define __LOADER_VCHROOT_H__

#include <cstddef>
#include <string>

// Inspired by (but not copied from) Darling's vchroot_expand.
// Takes a Haiku path in path, and writes a host path in hostPath.
// Returns the real host path length.
size_t loader_vchroot_expand(const char* path, char* hostPath, size_t size);

// Does the reverse.
size_t loader_vchroot_unexpand(const char* hostPath, char* path, size_t size);

// Takes an fd and a path, and returns the real host path.
// This function is useful due to the large number of Haiku syscalls
// taking an fd and a path as the first arguments.
size_t loader_vchroot_expandat(int fd, const char* path, char* hostPath, size_t size);
size_t loader_vchroot_unexpandat(int fd, const char* path, char* hostPath, size_t size);

bool loader_init_vchroot(const char* hprefix);

extern std::string gHaikuPrefix;

#endif // __LOADER_VCHROOT_H__