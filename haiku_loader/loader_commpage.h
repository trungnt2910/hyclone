#ifndef __LOADER_COMMPAGE_H__
#define __LOADER_COMMPAGE_H__

void* loader_allocate_commpage();
void loader_free_commpage(void* commpage);

#endif // __LOADER_COMMPAGE_H__