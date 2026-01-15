/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ricky Wu
 * All rights reserved.
 */

#ifndef _S4_ACTIVATOR_H_
#define _S4_ACTIVATOR_H_

#include <sys/elf64.h>

/*
 * FreeBSD S4 Hibernate Custom ELF Program Header Types
 */
#define PT_FREEBSD_S4_PCB        0x61000054  /* PCB (Process Control Block) */
#define PT_FREEBSD_S4_TRAMPOLINE 0x61000055  /* Resume trampoline code */

/*
 * FreeBSD S4 Hibernate Custom ELF Type
 */
#define ET_FREEBSD_S4_IMAGE      0x6100      /* S4 hibernate image */

/*
 * FreeBSD Swap Partition GUID
 * 516e7cb5-6ecf-11d6-8ff8-00022d09712b
 */
#define FREEBSD_SWAP_GUID \
	{ 0x516e7cb5, 0x6ecf, 0x11d6, \
	  { 0x8f, 0xf8, 0x00, 0x02, 0x2d, 0x09, 0x71, 0x2b } }

#endif /* _S4_ACTIVATOR_H_ */
