// SPDX-License-Identifier: GPL-2.0-only
#ifndef _RTL8372N_ERROR_H_
#define _RTL8372N_ERROR_H_

enum rtl8372n_error {
	RT_ERR_FAILED = -1,
	RT_ERR_OK = 0,
	RT_ERR_INPUT = 1,
	RT_ERR_NULL_POINTER = 7,
	RT_ERR_BUSYWAIT_TIMEOUT = 10,
	RT_ERR_CHIP_NOT_SUPPORTED = 13,
	RT_ERR_SMI = 14,
	RT_ERR_CHIP_NOT_FOUND = 16,
};

#endif
