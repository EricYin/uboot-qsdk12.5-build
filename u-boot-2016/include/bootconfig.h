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

#ifndef __BOOTCONFIG_H__
#define __BOOTCONFIG_H__

#define BOOTCONFIG_PART_NAME            "0:BOOTCONFIG"
#define BOOTCONFIG_BACKUP_PART_NAME     "0:BOOTCONFIG1"
#define ALT_PART_NAME_LENGTH            16
#define NUM_ALT_PARTITION               16
#define BOOTCONFIG_MAGIC_START          0xA3A2A1A0
#define BOOTCONFIG_MAGIC_START_TRYMODE  0xA3A2A1A1
#define BOOTCONFIG_MAGIC_END            0xB3B2B1B0

/* Custom bootconfig structure with packed attribute to ensure correct layout */
struct bootconfig_part_entry {
	char name[ALT_PART_NAME_LENGTH];
	uint32_t primaryboot;
} __attribute__ ((__packed__));

struct bootconfig_info {
	uint32_t magic_start;
	uint32_t age;
	uint32_t numaltpart;
	struct bootconfig_part_entry per_part_entry[NUM_ALT_PARTITION];
	uint32_t magic_end;
} __attribute__ ((__packed__));

typedef struct {
	const char *part_name; /* BOOTCONFIG 分区名 */
	ulong offset; /* BOOTCONFIG 分区偏移量 */
	size_t size; /* BOOTCONFIG 有效数据大小 */
	struct bootconfig_info info;
} bootconfig_info_t;

int get_bootconfig_part_offset(const char *part_name);
int read_bootconfig(bootconfig_info_t *bootcfg, bool skip_validation, bool quiet);
int read_bootconfig_and_write_back_if_needed(bootconfig_info_t *bootcfg);
int write_bootconfig(bootconfig_info_t *bootcfg);
int sync_bootconfig(bool reverse);
int print_bootconfig(void);
int get_bootconfig(const char *part_name);
int set_bootconfig(const char *part_name, uint32_t value);
bool validate_bootconfig(void);

#endif /* __BOOTCONFIG_H__ */
