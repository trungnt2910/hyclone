HYCLONE_SERVERCALL6(connect, int, int, intptr_t, intptr_t, intptr_t, intptr_t)
HYCLONE_SERVERCALL0(disconnect)
HYCLONE_SERVERCALL2(request_ack, int, int)
HYCLONE_SERVERCALL1(request_read, void*)
HYCLONE_SERVERCALL1(request_reply, intptr_t)
HYCLONE_SERVERCALL2(debug_output, const char*, size_t)
HYCLONE_SERVERCALL1(set_scheduler_mode, int)
HYCLONE_SERVERCALL0(get_scheduler_mode)
HYCLONE_SERVERCALL2(register_image, void*, size_t)
HYCLONE_SERVERCALL3(get_image_info, int, void*, size_t)
HYCLONE_SERVERCALL4(get_next_image_info, int, int*, void*, size_t)
HYCLONE_SERVERCALL1(unregister_image, int)
HYCLONE_SERVERCALL1(image_relocated, int)
HYCLONE_SERVERCALL1(shutdown, bool)
HYCLONE_SERVERCALL2(register_area, void*, unsigned int)
HYCLONE_SERVERCALL5(share_area, int, intptr_t, size_t, char*, size_t)
HYCLONE_SERVERCALL3(get_shared_area_path, int, char*, size_t)
HYCLONE_SERVERCALL5(transfer_area, int, void**, unsigned int, int, int)
HYCLONE_SERVERCALL2(get_area_info, int, void*)
HYCLONE_SERVERCALL4(get_next_area_info, int, void*, void*, void*)
HYCLONE_SERVERCALL1(unregister_area, int)
HYCLONE_SERVERCALL2(set_area_protection, int, unsigned int)
HYCLONE_SERVERCALL2(resize_area, int, size_t)
HYCLONE_SERVERCALL1(area_for, void*)
HYCLONE_SERVERCALL2(unmap_memory, void*, size_t)
HYCLONE_SERVERCALL3(set_memory_protection, void*, size_t, unsigned int)
HYCLONE_SERVERCALL3(set_memory_lock, void*, size_t, int)
HYCLONE_SERVERCALL3(register_entry_ref, unsigned long long, unsigned long long, int)
HYCLONE_SERVERCALL6(register_entry_ref_child, unsigned long long, unsigned long long, unsigned long long, unsigned long long, const char*, size_t)
HYCLONE_SERVERCALL5(get_entry_ref, unsigned long long, unsigned long long, void*, void*, bool)
HYCLONE_SERVERCALL1(fork, int)
HYCLONE_SERVERCALL1(exec, bool)
HYCLONE_SERVERCALL0(wait_for_fork_unlock)
HYCLONE_SERVERCALL3(create_port, int, const char*, size_t)
HYCLONE_SERVERCALL1(close_port, int)
HYCLONE_SERVERCALL1(delete_port, int)
HYCLONE_SERVERCALL2(find_port, const char*, size_t)
HYCLONE_SERVERCALL3(port_buffer_size_etc, int, unsigned int, unsigned long long)
HYCLONE_SERVERCALL2(get_port_info, int, void*)
HYCLONE_SERVERCALL3(get_next_port_info, int, int*, void*)
HYCLONE_SERVERCALL1(port_count, int)
HYCLONE_SERVERCALL2(set_port_owner, int, int)
HYCLONE_SERVERCALL6(read_port_etc, int, int*, void*, size_t, unsigned int, unsigned long long)
HYCLONE_SERVERCALL6(write_port_etc, int, int, const void*, size_t, unsigned int, unsigned long long)
HYCLONE_SERVERCALL5(get_port_message_info_etc, int, void*, size_t, unsigned int, unsigned long long)
HYCLONE_SERVERCALL3(create_sem, int, const char*, size_t)
HYCLONE_SERVERCALL1(acquire_sem, int)
HYCLONE_SERVERCALL4(acquire_sem_etc, int, unsigned int, unsigned int, unsigned long long)
HYCLONE_SERVERCALL1(release_sem, int)
HYCLONE_SERVERCALL3(release_sem_etc, int, unsigned int, unsigned int)
HYCLONE_SERVERCALL1(delete_sem, int)
HYCLONE_SERVERCALL2(get_sem_count, int, int*)
HYCLONE_SERVERCALL0(get_system_sem_count)
HYCLONE_SERVERCALL2(read_fs_info, int, void*)
HYCLONE_SERVERCALL1(next_device, int*)
HYCLONE_SERVERCALL6(mount, void*, void*, void*, unsigned int, const char*, size_t)
HYCLONE_SERVERCALL3(unmount, const char*, size_t, unsigned int)
HYCLONE_SERVERCALL6(read_stat, int, const char*, size_t, bool, void*, size_t)
HYCLONE_SERVERCALL6(write_stat, int, const char*, size_t, bool, const void*, int)
HYCLONE_SERVERCALL4(stat_attr, int, const char*, size_t, void*)
HYCLONE_SERVERCALL3(transform_dirent, int, void*, size_t)
HYCLONE_SERVERCALL5(register_fd, int, int, const char*, size_t, bool)
HYCLONE_SERVERCALL5(register_fd1, int, unsigned long long, unsigned long long, const char*, size_t)
HYCLONE_SERVERCALL4(register_parent_dir_fd, int, int, char*, size_t)
HYCLONE_SERVERCALL2(register_dup_fd, int, int)
HYCLONE_SERVERCALL5(register_attr_fd, int, int, void*, void*, bool)
HYCLONE_SERVERCALL1(unregister_fd, int)
HYCLONE_SERVERCALL4(setcwd, int, const char*, size_t, bool)
HYCLONE_SERVERCALL3(getcwd, char*, size_t, bool)
HYCLONE_SERVERCALL3(change_root, const char*, size_t, bool)
HYCLONE_SERVERCALL2(get_root, char*, size_t)
HYCLONE_SERVERCALL5(normalize_path, const char*, size_t, bool, char*, size_t)
HYCLONE_SERVERCALL6(vchroot_expandat, int, const char*, size_t, bool, char*, size_t)
HYCLONE_SERVERCALL6(get_attr_path, int, void*, void*, unsigned int, int, void*)
HYCLONE_SERVERCALL6(read_attr, int, const char*, size_t, size_t, void*, size_t)
HYCLONE_SERVERCALL6(write_attr, int, void*, unsigned int, size_t, const void*, size_t)
HYCLONE_SERVERCALL3(remove_attr, int, const char*, size_t)
HYCLONE_SERVERCALL4(ioctl, int, unsigned int, void*, size_t)
HYCLONE_SERVERCALL1(register_thread_info, void*)
HYCLONE_SERVERCALL2(get_thread_info, int, void*)
HYCLONE_SERVERCALL3(get_next_thread_info, int, int*, void*)
HYCLONE_SERVERCALL3(rename_thread, int, const char*, size_t)
HYCLONE_SERVERCALL2(set_thread_priority, int, int)
HYCLONE_SERVERCALL1(suspend_thread, int)
HYCLONE_SERVERCALL1(resume_thread, int)
HYCLONE_SERVERCALL0(wait_for_resume)
HYCLONE_SERVERCALL2(block_thread, int, unsigned long long)
HYCLONE_SERVERCALL2(unblock_thread, int, int)
HYCLONE_SERVERCALL4(send_data, int, int, const void*, size_t)
HYCLONE_SERVERCALL3(receive_data, int*, void*, size_t)
HYCLONE_SERVERCALL1(register_user_thread, void*)
HYCLONE_SERVERCALL2(get_team_info, int, void*)
HYCLONE_SERVERCALL2(get_next_team_info, int*, void*)
HYCLONE_SERVERCALL5(get_extended_team_info, int, unsigned int, void*, size_t, size_t*)
HYCLONE_SERVERCALL1(register_team_info, void*)
HYCLONE_SERVERCALL1(notify_loading_app, int)
HYCLONE_SERVERCALL1(wait_for_app_load, int)
HYCLONE_SERVERCALL2(register_messaging_service, int, int)
HYCLONE_SERVERCALL0(unregister_messaging_service)
HYCLONE_SERVERCALL4(start_watching_system, int, unsigned int, int, int)
HYCLONE_SERVERCALL4(stop_watching_system, int, unsigned int, int, int)
HYCLONE_SERVERCALL5(start_watching, unsigned long long, unsigned long long, unsigned int, int, unsigned int)
HYCLONE_SERVERCALL4(stop_watching, unsigned long long, unsigned long long, int, unsigned int)
HYCLONE_SERVERCALL2(stop_notifying, int, unsigned int)
HYCLONE_SERVERCALL4(get_safemode_option, const char*, size_t, char*, size_t*)
HYCLONE_SERVERCALL1(uid_for, intptr_t)
HYCLONE_SERVERCALL1(gid_for, intptr_t)
HYCLONE_SERVERCALL1(hostuid_for, int)
HYCLONE_SERVERCALL1(hostgid_for, int)
HYCLONE_SERVERCALL1(getuid, bool)
HYCLONE_SERVERCALL1(getgid, bool)
HYCLONE_SERVERCALL5(setregid, int, int, bool, intptr_t*, intptr_t*)
HYCLONE_SERVERCALL5(setreuid, int, int, bool, intptr_t*, intptr_t*)
HYCLONE_SERVERCALL2(getgroups, size_t, int*)
HYCLONE_SERVERCALL3(setgroups, size_t, const int*, intptr_t*)
HYCLONE_SERVERCALL1(install_default_debugger, int)
HYCLONE_SERVERCALL2(install_team_debugger, int, int)
HYCLONE_SERVERCALL3(register_nub, int, int, int)