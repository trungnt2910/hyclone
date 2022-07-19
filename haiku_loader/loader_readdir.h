#ifndef __LOADER_READDIR_H__
#define __LOADER_READDIR_H__

// These functions exist because Haiku's syscall requires
// a 4th parameter to _kern_read_dir, maxCount, which limits
// the number of entries read.
//
// Linux does not provide this option, therefore, over-reading
// a directory fd may cause directory entries to be left behind.

void loader_opendir(int fd);
void loader_closedir(int fd);
int loader_readdir(int fd, void* buffer, size_t bufferSize, int maxCount);

#endif