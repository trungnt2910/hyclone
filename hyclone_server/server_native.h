#ifndef __SERVER_NATIVE_H__
#define __SERVER_NATIVE_H__

#include <cstddef>

#include "haiku_thread.h"

size_t server_read_process_memory(int pid, void* address, void* buffer, size_t size);
size_t server_write_process_memory(int pid, void* address, const void* buffer, size_t size);

void server_kill_process(int pid);
void server_close_connection(intptr_t conn_id);

void server_fill_thread_info(haiku_thread_info* info);

#endif // __SERVER_NATIVE_H__