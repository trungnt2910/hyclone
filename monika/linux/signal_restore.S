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

#elif defined(__aarch64__)

__restore:
__restore_rt:
	mov x8,#139 // SYS_rt_sigreturn
	svc 0

#else

#	error Missing assembly!

#endif
