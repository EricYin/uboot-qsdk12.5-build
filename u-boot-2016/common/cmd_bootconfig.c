// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 chenxin527. All Rights Reserved.
 *
 * This file is part of the project uboot-qsdk12.5-build
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <command.h>
#include <bootconfig.h>

static int do_bootconfig_print(void)
{
	return print_bootconfig() ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

static int do_bootconfig_sync(bool reverse)
{
	return sync_bootconfig(reverse) ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

static int do_bootconfig_get(const char *part_name)
{
	int ret = get_bootconfig(part_name);

	if (ret < 0)
		return CMD_RET_FAILURE;

	if (ret > 1) {
		printf("Invalid primary boot value %d for %s\n", ret, part_name);
		return CMD_RET_FAILURE;
	}

	printf("%s = %d\n", part_name, ret);
	return CMD_RET_SUCCESS;
}

static int do_bootconfig_set(const char *part_name, uint32_t value)
{
	return set_bootconfig(part_name, value) ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

/**
 * do_bootconfig - Main bootconfig command handler
 */
static int do_bootconfig(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;

	/* bootconfig print */
	if (strcmp(argv[1], "print") == 0)
		return do_bootconfig_print();

	/* bootconfig sync */
	if (strcmp(argv[1], "sync") == 0) {
		if (argc > 2) {
			if (strcmp(argv[2], "reverse") == 0) {
				return do_bootconfig_sync(true);
			} else {
				puts("Usage: bootconfig sync [reverse]\n");
				return CMD_RET_USAGE;
			}
		} else {
			return do_bootconfig_sync(false);
		}
	}

	/* bootconfig get <name> */
	if (strcmp(argv[1], "get") == 0) {
		if (argc < 3) {
			puts("Usage: bootconfig get <partition_name>\n");
			return CMD_RET_USAGE;
		}
		return do_bootconfig_get(argv[2]);
	}

	/* bootconfig set <name|firmware|all> <0|1> */
	if (strcmp(argv[1], "set") == 0) {
		if (argc < 4) {
			puts("Usage: bootconfig set <partition_name|firmware|all> <0|1>\n");
			return CMD_RET_USAGE;
		}
		return do_bootconfig_set(argv[2], simple_strtoul(argv[3], NULL, 0));
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	bootconfig, 4, 0, do_bootconfig,
	"manage boot configuration",
	"bootconfig print           - Print all bootconfig information\n"
	"bootconfig sync [reverse]  - Sync 0:BOOTCONFIG1 with 0:BOOTCONFIG or vice versa\n"
	"bootconfig get <name>      - Get primaryboot value for a partition\n"
	"bootconfig set <name|firmware|all> <0|1> - Set primaryboot value for partition(s)\n"
	"\n"
	"Examples:\n"
	"  bootconfig print                    - Display all bootconfig info\n"
	"  bootconfig sync                     - Sync 0:BOOTCONFIG -> 0:BOOTCONFIG1\n"
	"  bootconfig sync reverse             - Sync 0:BOOTCONFIG1 -> 0:BOOTCONFIG\n"
	"  bootconfig get rootfs               - Get rootfs primaryboot value\n"
	"  bootconfig set rootfs 1             - Set rootfs primaryboot to 1\n"
	"  bootconfig set firmware 0           - Set firmware partitions (0:HLOS, rootfs, 0:WIFIFW) to 0\n"
	"  bootconfig set all 0                - Set all partitions to 0\n"
);
