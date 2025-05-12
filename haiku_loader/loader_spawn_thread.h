#ifndef __LOADER_SPAWN_THREAD_H__
#define __LOADER_SPAWN_THREAD_H__

struct thread_creation_attributes;

int loader_spawn_thread(void* thread_creation_attributes);
void loader_exit_thread(int retVal);
int loader_wait_for_thread(int threadId, uint32_t flags, int64_t timeout, int* retVal);
bool loader_register_thread(int thread_id, const thread_creation_attributes* attributes, bool suspended);

#endif // __LOADER_SPAWN_THREAD_H__
