// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 chenxin527. All Rights Reserved.
 *
 * This file is part of the project uboot-qsdk12.5-build
 *
 * Failsafe helper functions
 */

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <net/httpd.h>
#include <ipq_api.h>

#include "modules.h"

bool get_enable_state(const char *env_key, bool enable_by_default)
{
	if (!env_key || !env_key[0])
		return enable_by_default;

	const char *state_str = getenv(env_key);
	const char *disable_strs[] = {"0", "false", "no", "off"};

	if (!state_str)
		return enable_by_default;

	for (int i = 0; i < ARRAY_SIZE(disable_strs); i++)
		if (!strcasecmp(state_str, disable_strs[i]))
			return false;

	return true;
}
