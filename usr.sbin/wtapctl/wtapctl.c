/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 The FreeBSD Foundation
 *
 * This software was developed by En-Wei Wu under sponsorship from 
 * the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright 
 *	 notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *	 notice, this list of conditions and the following disclaimer in the 
 *	 documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "wtapctl.h"

#include "if_wtapioctl.h"
#include "plugins/visibility_ioctl.h"

static int dev_fd = -1;
static int vis_fd = -1;
static struct cmd *cmds = NULL;

static void
usage(void)
{
	fprintf(stderr, "usage: %s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n", 
		"wtapctl device create [id]",
		"wtapctl device delete <id>",
		"wtapctl device list",
		"wtapctl vis [open | close]",
		"wtapctl vis [add | delete] <id from> <id to>",
		"wtapctl vis show <id>");
	exit(1);
}

static void
device_create(const struct cmd *cmd __unused, int id)
{
	int old_id = id;

	if (ioctl(dev_fd, WTAPIOCTLCRT, &id) < 0)
		errx(1, "error creating wtap with id=%d", id);

	if (old_id != id)
		printf("wtap%d\n", id);
}

static void
device_delete(const struct cmd *cmd __unused, int id)
{
	if (ioctl(dev_fd, WTAPIOCTLDEL, &id) < 0)
		errx(1, "error deleting wtap with id=%d", id);
}

static void
device_list(const struct cmd *cmd __unused)
{
	uint64_t devs_set;
	int id;

	if (ioctl(dev_fd, WTAPIOCTLLIST, &devs_set) < 0)
		errx(1, "error getting wtap device list");

	for (id = 0; id < MAX_NBR_WTAP; id++)
		if (isset(&devs_set, id))
			printf("wtap%d ", id);

	printf("\n");
}

static void
vis_toggle_medium(const struct cmd *cmd)
{
	int op;

	if (strcmp(cmd->c_name, "open") == 0)
		op = 1;
	else
		op = 0;

	if (ioctl(vis_fd, VISIOCTLSETOPEN, &op) < 0)
		errx(1, "error %s medium\n", (op == 1 ? "opening": "closing"));
}


static void
vis_link_op(const struct cmd *cmd, int id1, int id2)
{
	struct link l;

	if (strcmp(cmd->c_name, "add") == 0)
		l.op = 1;
	else
		l.op = 0;

	l.id1 = id1;
	l.id2 = id2;

	if (ioctl(vis_fd, VISIOCTLSETLINK, &l) < 0)
		errx(1, "error making a link operation");
}

static void
vis_link_show(const struct cmd *cmd __unused, int id)
{
	int is_open, i, j;
	struct vis_map_req req;

	if (id < 0 || id >= MAX_NBR_WTAP)
		errx(1, "device id must be between 0 and 63");

	req.id = id;

	if (ioctl(vis_fd, VISIOCTLGETOPEN, &is_open) < 0)
		errx(1, "error getting medium state");

	if (ioctl(vis_fd, VISIOCTLGETMAP, &req) < 0)
		errx(1, "error getting link information of id=%d", id);

	printf("medium: %s\n", is_open ? "open": "close");

	printf("wtap%d -> ", id);

	for (i = 0; i < ARRAY_SIZE; i++) {
		uint32_t vis = req.map[i];

		for (j = 0; j < 32; j++) {
			if (vis & 0x1)
				printf("wtap%d ", i * ARRAY_SIZE + j);

			vis >>= 1;
		}
	}

	printf("\n");
}

static struct cmd *
find_command(const char *cmdstr)
{
	int i;
	struct cmd *cmd;

	for (i = 0; ; i++) {
		cmd = &cmds[i];

		/* Sentinel */
		if (cmd->c_name == NULL)
			return NULL;

		if (strcmp(cmd->c_name, cmdstr) == 0)
			return cmd;
	}

	return NULL;
}

static void
run_command(const struct cmd *cmd, int argc, const char *argv[])
{
	int arg;

	switch (cmd->c_arg_flag) {
	case ARG_NOARG:
		if (argc != 0)
			usage();
		
		cmd->c_func_noarg(cmd);
		break;
	case ARG_NEXTARGOPT:
		if (argc > 1)
			usage();
		
		arg = argc ? atoi(argv[0]) : -1;
		cmd->c_func_arg(cmd, arg);
		break;
	case ARG_NEXTARG:
		if (argc != 1)
			usage();
		
		cmd->c_func_arg(cmd, atoi(argv[0]));
		break;
	case ARG_NEXTARG2:
		if (argc != 2)
			usage();
		
		cmd->c_func_arg2(cmd, atoi(argv[0]), atoi(argv[1]));
		break;
	default:
		errx(1, "internal fault");
	}
}

static struct cmd dev_cmds[] = {
	DEF_CMD_ARGOPT("create", device_create),
	DEF_CMD_ARG("delete", device_delete),
	DEF_CMD_NOARG("list", device_list),
	CMD_SENTINEL
};

static struct cmd vis_cmds[] = {
	DEF_CMD_NOARG("open", vis_toggle_medium),
	DEF_CMD_NOARG("close", vis_toggle_medium),
	DEF_CMD_ARG2("add", vis_link_op),
	DEF_CMD_ARG2("delete", vis_link_op),
	DEF_CMD_ARG("show", vis_link_show),
	CMD_SENTINEL
};

int 
main(int argc, const char *argv[])
{
	if (argc < 3)
		usage();

	struct cmd *cmd;

	if (strcmp(argv[1], "device") == 0) {
		cmds = dev_cmds;
	} else if (strcmp(argv[1], "vis") == 0) {
		cmds = vis_cmds;
	} else {
		usage();
	}

	cmd = find_command(argv[2]);
	if (!cmd)
		usage();

	argc -= 3;
	argv += 3;

	/* We defer opening file descriptor until now */
	dev_fd = open(WTAP_DEV_NODE, O_RDONLY);
	if (dev_fd < 0)
		errx(1, "error opening %s", WTAP_DEV_NODE);

	vis_fd = open(VIS_DEV_NODE, O_RDONLY);
	if (vis_fd < 0)
		errx(1, "error opening %s", VIS_DEV_NODE);

	run_command(cmd, argc, argv);

	return 0;
}
