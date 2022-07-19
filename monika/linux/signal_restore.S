.text
.global __restore_rt
.hidden __restore_rt
.global __restore
.hidden __restore

#if defined(__x86_64__)

__restore_rt:
__restore:
	movl $15, %eax
	syscall

#elif defined(__i386__)

__restore:
	popl %eax
	movl $119, %eax
	int $0x80

__restore_rt:
	movl $173, %eax
	int $0x80

#else

#	error Missing assembly!

#endif