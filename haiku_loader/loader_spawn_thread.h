#ifndef __LOADER_SPAWN_THREAD_H__
#define __LOADER_SPAWN_THREAD_H__

int loader_spawn_thread(void* thread_creation_attributes);
void loader_exit_thread(int retVal);
int loader_wait_for_thread(int thread_id, int* retVal);

#endif // __LOADER_SPAWN_THREAD_H__