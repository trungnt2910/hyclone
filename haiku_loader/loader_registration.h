#ifndef __LOADER_REGISTRATION_H__
#define __LOADER_REGISTRATION_H__

bool loader_register_process(int argc, char** args);
bool loader_register_builtin_areas(void* commpage, char** args);

#endif // __LOADER_REGISTRATION_H__