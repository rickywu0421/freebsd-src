/*-
 * Copyright (c) 2010-2011 Monthadar Al Jaberi, TerraNet AB
 * All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <lib80211/lib80211_ioctl.h>

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("usage: %s <interface>\n", argv[0]);
		exit(1);
	}

	int fd = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (fd == -1) {
		printf("error opening socket\n");
		exit(1);
	}

	char *temp;
	struct sockaddr_dl sdl;
	struct ieee80211req_mlme mlme;

	temp = malloc(strlen(val) + 2); /* ':' and '\0' */
	if (temp == NULL)
		errx(1, "malloc failed");
	temp[0] = ':';
	strcpy(temp + 1, val);
	sdl.sdl_len = sizeof(sdl);
	link_addr(temp, &sdl);
	free(temp);
	if (sdl.sdl_alen != IEEE80211_ADDR_LEN)
		errx(1, "malformed link-level address");

	mlme.im_op = IEEE80211_MLME_ASSOC;
	memcpy(mlme.im_macaddr, LLADDR(&sdl), IEEE80211_ADDR_LEN);
	
	set80211(s, IEEE80211_IOC_MLME, 0,
		sizeof(mlme), &mlme);
}