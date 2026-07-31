// SPDX-License-Identifier: GPL-2.0-only
#ifndef _RTL8372N_TYPES_H_
#define _RTL8372N_TYPES_H_

#include <common.h>

typedef unsigned int rtk_uint32;
typedef int rtk_int32;
typedef unsigned short rtk_uint16;
typedef unsigned char rtk_uint8;

typedef rtk_int32 rtk_api_ret_t;
typedef rtk_int32 ret_t;

#define rtl_debug(fmt, ...) printf("rtl8372n: " fmt, ##__VA_ARGS__)

#endif
