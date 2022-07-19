.text
.globl linux_syscall
.hidden linux_syscall

linux_syscall:

#if defined(__x86_64__)

	movl	8(%rsp), %eax
	movq	%rcx, %r10
	syscall
	ret

#elif defined(__i386__)

	// eax, ecx and edx are caller saved, the rest is calee saved
	push	%ebx
	push	%esi
	push	%edi
	push	%ebp

	mov		20(%esp), %ebx
	mov		24(%esp), %ecx
	mov		28(%esp), %edx
	mov		32(%esp), %esi
	mov		36(%esp), %edi
	mov		40(%esp), %ebp
	mov		44(%esp), %eax

	int		$0x80

	pop		%ebp
	pop		%edi
	pop		%esi
	pop		%ebx
	ret

#else

#	error Missing assembly!

#endif