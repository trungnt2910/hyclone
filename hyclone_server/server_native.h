#ifndef __SERVER_NATIVE_H__
#define __SERVER_NATIVE_H__

#include <cstddef>
#include <filesystem>

#include "haiku_team.h"
#include "haiku_thread.h"

struct haiku_stat;
struct haiku_fs_info;

size_t server_read_process_memory(int pid, void* address, void* buffer, size_t size);
size_t server_write_process_memory(int pid, void* address, const void* buffer, size_t size);

void server_kill_process(int pid);
void server_close_connection(intptr_t conn_id);

void server_fill_team_info(haiku_team_info* info);
void server_fill_thread_info(haiku_thread_info* info);

void server_fill_fs_info(const std::filesystem::path& path, haiku_fs_info* info);

status_t server_read_stat(const std::filesystem::path& path, haiku_stat& st);
status_t server_write_stat(const std::filesystem::path& path, const haiku_stat& stat, int statMask);

#endif // __SERVER_NATIVE_H__