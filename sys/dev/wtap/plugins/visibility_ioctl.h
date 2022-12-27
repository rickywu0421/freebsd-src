/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2011 Monthadar Al Jaberi, TerraNet AB
 * All rights reserved.
 *
 * Copyright (c) 2023 The FreeBSD Foundation
 *
 * Portions of this software were developed by En-Wei Wu
 * under sponsorship from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
 *    redistribution must be conditioned upon including a substantially
 *    similar Disclaimer requirement for further binary redistribution.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGES.
 *
 * $FreeBSD$
 */

/*
 * Ioctl-related defintions for the Wireless TAP plugins.
 */

#ifndef _VISIBILITY_IOCTL_H
#define _VISIBILITY_IOCTL_H

#include <sys/param.h>

#ifndef MAX_NBR_WTAP
#define MAX_NBR_WTAP (64) // We support a maximum of 64 nodes for now
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE (MAX_NBR_WTAP / (int)(sizeof(uint32_t) * NBBY))
#endif

struct vis_map_req {
      int id;
      uint32_t map[ARRAY_SIZE];
};

struct link {
      int	op; //0 remove, 1 link
      int 	id1;
      int	id2;
};

#define VISIOCTLSETOPEN _IOW('W', 4, int) // 0 close, 1 open
#define VISIOCTLSETLINK _IOW('W', 5, struct link) //
#define VISIOCTLGETOPEN _IOR('R', 6, int)
#define VISIOCTLGETMAP  _IOWR('R', 7, struct vis_map_req)

#endif
