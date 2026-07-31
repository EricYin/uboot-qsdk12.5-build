#ifndef _RTL8372N_BOOT_IO_H_
#define _RTL8372N_BOOT_IO_H_

#include "rtl8372n_types.h"

ret_t rtl8372n_phy_read(rtk_uint32 port, rtk_uint32 page,
			 rtk_uint32 reg, rtk_uint32 *value);
ret_t rtl8372n_phy_write(rtk_uint32 port_mask, rtk_uint32 page,
			  rtk_uint32 reg, rtk_uint32 value);
ret_t rtl8372n_phy_regbits_read(rtk_uint32 port, rtk_uint32 page,
				 rtk_uint32 reg, rtk_uint32 bits,
				 rtk_uint32 *value);
ret_t rtl8372n_phy_regbits_write(rtk_uint32 port_mask, rtk_uint32 page,
				  rtk_uint32 reg, rtk_uint32 bits,
				  rtk_uint32 value);
ret_t uc1_sram_write_8b(rtk_uint32 port, rtk_uint32 addr,
			 rtk_uint32 value);
rtk_uint32 uc1_sram_read_8b(rtk_uint32 port, rtk_uint32 addr);
ret_t uc2_sram_write_8b(rtk_uint32 port, rtk_uint32 addr,
			 rtk_uint32 value);
ret_t data_ram_write_8b(rtk_uint8 port, rtk_uint32 addr,
			 rtk_uint32 value);

ret_t rtl8372n_sds_reg_read(rtk_uint32 sds, rtk_uint32 reg,
			     rtk_uint32 page, rtk_uint32 *value);
ret_t rtl8372n_sds_reg_write(rtk_uint32 sds, rtk_uint32 reg,
			      rtk_uint32 page, rtk_uint32 value);
ret_t rtl8372n_sds_regbits_read(rtk_uint32 sds, rtk_uint32 reg,
				 rtk_uint32 page, rtk_uint32 bits,
				 rtk_uint32 *value);
ret_t rtl8372n_sds_regbits_write(rtk_uint32 sds, rtk_uint32 reg,
				  rtk_uint32 page, rtk_uint32 bits,
				  rtk_uint32 value);
ret_t fw_reset_flow_tgr(rtk_uint32 sds);

#endif
