#ifndef _RTL8372N_SWITCH_H_
#define _RTL8372N_SWITCH_H_

#include "rtl8372n_types.h"

typedef enum {
	CHIP_RTL8373 = 0,
	CHIP_RTL8372,
	CHIP_RTL8224,
	CHIP_RTLXXXX,
	CHIP_RTL8373N,
	CHIP_RTL8372N,
	CHIP_RTL8224N,
	CHIP_RTL8366,
	CHIP_END,
} switch_chip_t;

typedef enum {
	INIT_NOT_COMPLETED = 0,
	INIT_COMPLETED,
	INIT_STATE_END,
} init_state_t;

rtk_api_ret_t rtk_switch_initialState_set(init_state_t state);
rtk_api_ret_t rtk_switch_init(void);
rtk_api_ret_t rtk_switch_init_by_chip(switch_chip_t switchChip);

#endif
