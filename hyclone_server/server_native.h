#ifndef __SERVER_NATIVE_H__
#define __SERVER_NATIVE_H__

#include <cstddef>

size_t server_read_process_memory(int pid, void* address, void* buffer, size_t size);
size_t server_write_process_memory(int pid, void* address, const void* buffer, size_t size);

void server_kill_process(int pid);
void server_close_connection(intptr_t conn_id);

#endif // __SERVER_NATIVE_H__