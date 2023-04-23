#ifndef __LOADER_REGISTRATION_H__
#define __LOADER_REGISTRATION_H__

struct user_space_program_args;

bool loader_register_process(int argc, char** args);
bool loader_register_builtin_areas(user_space_program_args* args, void* commpage);
bool loader_register_existing_fds();
bool loader_register_groups();

#endif // __LOADER_REGISTRATION_H__