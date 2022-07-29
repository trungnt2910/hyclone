#ifndef __LOADER_RESERVEDRANGE_H__
#define __LOADER_RESERVEDRANGE_H__

#include <cstddef>

void loader_lock_reserved_range_data();
void loader_unlock_reserved_range_data();
void loader_register_reserved_range(void* address, size_t size);
void loader_unregister_reserved_range(void* address, size_t size);
void loader_map_reserved_range(void* address, size_t size);
void loader_unmap_reserved_range(void* address, size_t size);
bool loader_is_in_reserved_range(void* address, size_t size);
size_t loader_reserved_range_longest_mappable_from(void* address, size_t size);
size_t loader_next_reserved_range(void* address, void** nextAddress);

#endif // __LOADER_RESERVEDRANGE_H__