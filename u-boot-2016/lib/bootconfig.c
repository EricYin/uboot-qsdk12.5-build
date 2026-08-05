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
#include <mmc.h>
#include <sdhci.h>
#include <malloc.h>
#include <errno.h>
#include <ipq_api.h>
#include <flashrw.h>
#include <bootconfig.h>

#ifndef CONFIG_SDHCI_SUPPORT
extern qca_mmc mmc_host;
#else
extern struct sdhci_host mmc_host;
#endif

static const qca_smem_flash_info_t *sfi = &qca_smem_flash_info;

/**
 * get_bootconfig_part_offset - Get bootconfig partition offset
 */
int get_bootconfig_part_offset(const char *part_name)
{
	block_dev_desc_t *mmc_dev;
	disk_partition_t disk_info = {0};
	uint32_t offset_bytes = 0, size_bytes = 0;
	int ret;

	switch (sfi->flash_type) {
	case SMEM_BOOT_NAND_FLASH:
	case SMEM_BOOT_NOR_FLASH:
	case SMEM_BOOT_NORPLUSEMMC:
	case SMEM_BOOT_NORPLUSNAND:
	case SMEM_BOOT_ONENAND_FLASH:
	case SMEM_BOOT_QSPI_NAND_FLASH:
	case SMEM_BOOT_SPI_FLASH:
		ret = getpart_offset_size((char *)part_name, &offset_bytes, &size_bytes);
		if (ret)
			return -ENOENT;
		return offset_bytes;
	case SMEM_BOOT_MMC_FLASH:
	case SMEM_BOOT_NO_FLASH:
	case SMEM_BOOT_SDC_FLASH:
		if (!has_mmc())
			return -ENODEV;
		mmc_dev = mmc_get_dev(mmc_host.dev_num);
		if (!mmc_dev)
			return -ENODEV;
		ret = get_partition_info_efi_by_name(mmc_dev, (char *)part_name, &disk_info);
		if (ret)
			return -ENOENT;
		return disk_info.start * disk_info.blksz;
	default:
		puts("Unsupported flash type\n");
		return -EINVAL;
	}
}

/**
 * read_bootconfig - Read bootconfig data to bootcfg structure
 */
int read_bootconfig(bootconfig_info_t *bootcfg, bool skip_validation, bool quiet)
{
	int offset_bytes, ret;

	offset_bytes = get_bootconfig_part_offset(bootcfg->part_name);
	if (offset_bytes < 0) {
		if (!quiet)
			printf("Partition %s not found\n", bootcfg->part_name);
		return -ENOENT;
	}

	bootcfg->offset = offset_bytes;

	switch (sfi->flash_type) {
	case SMEM_BOOT_NOR_FLASH:
	case SMEM_BOOT_NORPLUSEMMC:
	case SMEM_BOOT_NORPLUSNAND:
	case SMEM_BOOT_SPI_FLASH:
		ret = read_data_from_spi(bootcfg->offset,
			bootcfg->size, &bootcfg->info, bootcfg->size);
		break;
	case SMEM_BOOT_NAND_FLASH:
	case SMEM_BOOT_ONENAND_FLASH:
	case SMEM_BOOT_QSPI_NAND_FLASH:
		ret = read_data_from_nand(bootcfg->offset,
			bootcfg->size, &bootcfg->info, bootcfg->size);
		break;
	case SMEM_BOOT_MMC_FLASH:
	case SMEM_BOOT_NO_FLASH:
	case SMEM_BOOT_SDC_FLASH:
		ret = read_data_from_mmc(bootcfg->offset,
			bootcfg->size, &bootcfg->info, bootcfg->size);
		break;
	default:
		if (!quiet)
			puts("Unsupported flash type\n");
		return -EINVAL;
	}

	if (ret) {
		if (!quiet)
			printf("Failed to read bootconfig data from partition %s\n", bootcfg->part_name);
		return ret;
	}

	if (skip_validation)
		return 0;

	/* Validate magic numbers */
	if (bootcfg->info.magic_start != BOOTCONFIG_MAGIC_START &&
	    bootcfg->info.magic_start != BOOTCONFIG_MAGIC_START_TRYMODE) {
		if (!quiet)
			printf("Invalid magic_start: 0x%08x in %s\n", bootcfg->info.magic_start, bootcfg->part_name);
		return -EINVAL;
	}

	if (bootcfg->info.magic_end != BOOTCONFIG_MAGIC_END) {
		if (!quiet)
			printf("Invalid magic_end: 0x%08x in %s\n", bootcfg->info.magic_end, bootcfg->part_name);
		return -EINVAL;
	}

	if (bootcfg->info.numaltpart > NUM_ALT_PARTITION) {
		if (!quiet)
			printf("Warning: numaltpart (%u) exceeds maximum (%d), truncating\n",
				bootcfg->info.numaltpart, NUM_ALT_PARTITION);
		bootcfg->info.numaltpart = NUM_ALT_PARTITION;
	}

	return 0;
}

int read_bootconfig_and_write_back_if_needed(bootconfig_info_t *bootcfg)
{
	bootconfig_info_t backup_bootcfg;
	int ret;

	bootcfg->part_name = BOOTCONFIG_PART_NAME;
	bootcfg->size = sizeof(struct bootconfig_info);

	ret = read_bootconfig(bootcfg, false, false);
	if (ret == 0 || ret == -ENOENT)
		return ret;

	backup_bootcfg.part_name = BOOTCONFIG_BACKUP_PART_NAME;
	backup_bootcfg.size = sizeof(struct bootconfig_info);

	printf("Read bootconfig data from partition %s\n", backup_bootcfg.part_name);

	ret = read_bootconfig(&backup_bootcfg, false, false);
	if (ret) {
		puts("Bootconfig data corrupted, please restore manually\n");
		return ret;
	}

	printf("Write bootconfig data back to partition %s\n", bootcfg->part_name);
	memcpy(&bootcfg->info, &backup_bootcfg.info, backup_bootcfg.size);
	return write_bootconfig(bootcfg);
}

/**
 * write_bootconfig - Write bootconfig data back to partition
 */
int write_bootconfig(bootconfig_info_t *bootcfg)
{
	int ret;

	switch (sfi->flash_type) {
	case SMEM_BOOT_NOR_FLASH:
	case SMEM_BOOT_NORPLUSEMMC:
	case SMEM_BOOT_NORPLUSNAND:
	case SMEM_BOOT_SPI_FLASH:
		ret = write_data_to_spi(bootcfg->offset, bootcfg->size, &bootcfg->info);
		break;
	case SMEM_BOOT_NAND_FLASH:
	case SMEM_BOOT_ONENAND_FLASH:
	case SMEM_BOOT_QSPI_NAND_FLASH:
		ret = write_data_to_nand(bootcfg->offset, bootcfg->size, &bootcfg->info);
		break;
	case SMEM_BOOT_MMC_FLASH:
	case SMEM_BOOT_NO_FLASH:
	case SMEM_BOOT_SDC_FLASH:
		ret = write_data_to_mmc(bootcfg->offset, bootcfg->size, &bootcfg->info);
		break;
	default:
		puts("Unsupported flash type\n");
		return -EINVAL;
	}

	if (ret)
		printf("Failed to write bootconfig data back to partition %s\n", bootcfg->part_name);

	return ret;
}

/**
 * sync_bootconfig - Sync 0:BOOTCONFIG1 with 0:BOOTCONFIG or vice versa
 */
int sync_bootconfig(bool reverse)
{
	bootconfig_info_t src, dst;
	int ret;

	/* Check if 0:BOOTCONFIG1 partition exists */
	ret = get_bootconfig_part_offset(BOOTCONFIG_BACKUP_PART_NAME);
	if (ret < 0) {
		printf("Partition %s does not exist, skip sync\n", BOOTCONFIG_BACKUP_PART_NAME);
		return 0;
	}

	if (reverse) {
		/* 0:BOOTCONFIG1 -> 0:BOOTCONFIG */
		src.part_name = BOOTCONFIG_BACKUP_PART_NAME;
		dst.part_name = BOOTCONFIG_PART_NAME;
	} else {
		/* 0:BOOTCONFIG -> 0:BOOTCONFIG1 */
		src.part_name = BOOTCONFIG_PART_NAME;
		dst.part_name = BOOTCONFIG_BACKUP_PART_NAME;
	}

	src.size = sizeof(struct bootconfig_info);
	dst.size = sizeof(struct bootconfig_info);

	/* Read source BOOTCONFIG partition */
	ret = read_bootconfig(&src, false, false);
	if (ret)
		return ret;

	/* Read destination BOOTCONFIG partition */
	ret = read_bootconfig(&dst, true, false);
	if (ret)
		return ret;

	printf("Syncing %s -> %s: ", src.part_name, dst.part_name);

	/* Compare the two bootconfig structures */
	ret = memcmp(&dst.info, &src.info, src.size);
	if (!ret) {
		puts("already in sync\n\n");
		return 0;
	}

	/* They are different, need to sync */
	memcpy(&dst.info, &src.info, src.size);

	ret = write_bootconfig(&dst);
	if (!ret)
		puts("success\n\n");
	else
		printf("failure (errno: %d)\n\n", ret);

	return ret;
}

/**
 * print_bootconfig - Print all bootconfig information
 */
int print_bootconfig(void)
{
	bootconfig_info_t bootcfg;
	int ret;

	ret = read_bootconfig_and_write_back_if_needed(&bootcfg);
	if (ret)
		return ret;

	puts("\n========== Bootconfig Information ==========\n");
	printf("Magic Start:      0x%08x %s\n", bootcfg.info.magic_start,
	       bootcfg.info.magic_start == BOOTCONFIG_MAGIC_START ? "(Normal)" :
	       bootcfg.info.magic_start == BOOTCONFIG_MAGIC_START_TRYMODE ? "(Try Mode)" : "(Unknown)");
	printf("Magic End:        0x%08x\n", bootcfg.info.magic_end);
	printf("Age:              0x%08x\n", bootcfg.info.age);
	printf("Number of Alt Partitions: %u\n", bootcfg.info.numaltpart);
	printf("\n%-3s %-16s %s\n", "Idx", "Partition Name", "Primary Boot");
	printf("--------------------------------------------\n");

	for (int i = 0; i < bootcfg.info.numaltpart && i < NUM_ALT_PARTITION; i++) {
		printf("%3d: %-16s %u\n", i, bootcfg.info.per_part_entry[i].name,
		       bootcfg.info.per_part_entry[i].primaryboot);
	}
	puts("============================================\n\n");

	return 0;
}

/**
 * get_bootconfig - Get primaryboot value for a partition
 */
int get_bootconfig(const char *part_name)
{
	bootconfig_info_t bootcfg;
	int ret;

	ret = read_bootconfig_and_write_back_if_needed(&bootcfg);
	if (ret)
		return ret;

	for (int i = 0; i < bootcfg.info.numaltpart && i < NUM_ALT_PARTITION; i++) {
		if (strncmp(bootcfg.info.per_part_entry[i].name, part_name,
			    ALT_PART_NAME_LENGTH) == 0) {
			return bootcfg.info.per_part_entry[i].primaryboot;
		}
	}

	printf("Entry '%s' not found\n", part_name);
	return -ENOENT;
}

/**
 * set_bootconfig - Set primaryboot value for partition(s)
 */
int set_bootconfig(const char *name, uint32_t value)
{
	bootconfig_info_t bootcfg;
	int idx, modified = 0;
	int ret;

	if (value != 0 && value != 1) {
		printf("Invalid value: %u (must be 0 or 1)\n", value);
		return -EINVAL;
	}

	ret = read_bootconfig_and_write_back_if_needed(&bootcfg);
	if (ret)
		return ret;

	if (strcmp(name, "all") == 0) {
		/* Handle "all" special case */
		for (idx = 0; idx < bootcfg.info.numaltpart && idx < NUM_ALT_PARTITION; idx++) {
			if (bootcfg.info.per_part_entry[idx].primaryboot != value) {
				bootcfg.info.per_part_entry[idx].primaryboot = value;
				modified++;
				printf("Set %s to %u\n", bootcfg.info.per_part_entry[idx].name, value);
			} else {
				printf("%s already %u\n", bootcfg.info.per_part_entry[idx].name, value);
			}
		}
		if (modified == 0)
			printf("All partitions already have value %u, no change\n", value);
	} else if (strcmp(name, "firmware") == 0) {
		/* Handle firmware partitions */
		const char *fw_parts[] = {
			"0:HLOS", "rootfs", "0:WIFIFW"
#ifdef CONFIG_ARCH_IPQ5018
			, "0:BTFW"
#endif
		};
		for (idx = 0; idx < bootcfg.info.numaltpart && idx < NUM_ALT_PARTITION; idx++) {
			for (int j = 0; j < ARRAY_SIZE(fw_parts); j++) {
				if (strncmp(bootcfg.info.per_part_entry[idx].name, fw_parts[j],
						ALT_PART_NAME_LENGTH) == 0) {
					if (bootcfg.info.per_part_entry[idx].primaryboot != value) {
						bootcfg.info.per_part_entry[idx].primaryboot = value;
						modified++;
						printf("Set %s to %u\n", fw_parts[j], value);
					} else {
						printf("%s already %u\n", fw_parts[j], value);
					}
					break;
				}
			}
		}
		if (modified == 0)
			printf("All firmware partitions already have value %u, no change\n", value);
	} else {
		/* Handle specific partition entry */
		for (idx = 0; idx < bootcfg.info.numaltpart && idx < NUM_ALT_PARTITION; idx++) {
			if (strncmp(bootcfg.info.per_part_entry[idx].name, name,
				    ALT_PART_NAME_LENGTH) == 0) {
				if (bootcfg.info.per_part_entry[idx].primaryboot != value) {
					bootcfg.info.per_part_entry[idx].primaryboot = value;
					modified = 1;
					printf("Set %s to %u\n", name, value);
				} else {
					printf("%s already %u, no change\n", name, value);
				}
				break;
			}
		}
		if (idx >= bootcfg.info.numaltpart || idx >= NUM_ALT_PARTITION) {
			printf("Partition entry '%s' not found\n", name);
			return -ENOENT;
		}
	}

	/* Only write back if modifications were made */
	if (modified) {
		ret = write_bootconfig(&bootcfg);
		if (ret)
			return ret;
		puts("\nBootconfig updated successfully\n\n");
	}

	/* Always sync 0:BOOTCONFIG -> 0:BOOTCONFIG1 */
	return sync_bootconfig(false);
}
