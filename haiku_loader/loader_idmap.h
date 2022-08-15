#ifndef __LOADER_IDMAP_H__
#define __LOADER_IDMAP_H__

void* loader_idmap_create();
void loader_idmap_destroy(void* idmap);
int loader_idmap_add(void* idmap, void* data);
void* loader_idmap_get(void* idmap, int id);
void loader_idmap_remove(void* idmap, int id);

#endif // __LOADER_IDMAP_H__
