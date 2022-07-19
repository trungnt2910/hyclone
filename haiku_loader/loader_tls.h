#ifndef __LOADER_TLS_H__
#define __LOADER_TLS_H__

#include <cstdint>

int32_t tls_allocate();
void* tls_get(int32_t index);
void** tls_address(int32_t index);
void tls_set(int32_t index, void* value);

void loader_init_tls();

#endif // __LOADER_TLS_H__