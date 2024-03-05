/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _ARCH_ARM64_SIGNAL_H_
#define _ARCH_ARM64_SIGNAL_H_


/*
 * Architecture-specific structure passed to signal handlers
 */

#if defined(__aarch64__)
struct vregs
{
	unsigned long	x[30];
	unsigned long	lr;
	unsigned long	sp;
	unsigned long	elr;
	unsigned long	spsr;
	__uint128_t	    fp_q[32];
	unsigned int	fpsr;
	unsigned int	fpcr;
};
#endif /* defined(__aarch64__) */


#endif /* _ARCH_ARM64_SIGNAL_H_ */
