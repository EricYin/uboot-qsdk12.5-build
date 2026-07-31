#include "include/rtk_error.h"
#include "include/rtl8372n_asicdrv.h"
#include "include/rtl8372n_boot_io.h"

#define RTL8372N_PHY_PORT_SELECT	0x6438
#define RTL8372N_PHY_DATA		0x6444
#define RTL8372N_PHY_DATA_LOW		0x6440
#define RTL8372N_PHY_CTRL		0x643c
#define RTL8372N_PHY_OP_STATUS		0x07000000
#define RTL8372N_PHY_READ_CMD		0x3
#define RTL8372N_PHY_WRITE_CMD		0x7
#define RTL8372N_IO_RETRIES		1000

#define RTL8372N_SDS_CTRL		0x03f8
#define RTL8372N_SDS_READ_DATA		0x03fc
#define RTL8372N_SDS_WRITE_DATA		0x0400

#define RTL8372N_PHY_VENDOR_PAGE	0x1f

static int rtl8372n_mask_shift(rtk_uint32 bits)
{
	int shift;

	if (!bits)
		return -1;
	for (shift = 0; shift < 32; shift++)
		if (bits & (1U << shift))
			return shift;
	return -1;
}

static ret_t rtl8372n_phy_wait_ready(void)
{
	rtk_uint32 busy, status;
	ret_t ret;
	int retry;

	for (retry = 0; retry < RTL8372N_IO_RETRIES; retry++) {
		ret = rtl8372n_getAsicRegBit(RTL8372N_PHY_CTRL, 0, &busy);
		if (ret != RT_ERR_OK)
			return ret;
		ret = rtl8372n_getAsicRegBits(RTL8372N_PHY_CTRL,
						  RTL8372N_PHY_OP_STATUS,
						  &status);
		if (ret != RT_ERR_OK)
			return ret;
		if (!busy && !status)
			return RT_ERR_OK;
	}

	return RT_ERR_BUSYWAIT_TIMEOUT;
}

ret_t rtl8372n_phy_read(rtk_uint32 port, rtk_uint32 page,
			 rtk_uint32 reg, rtk_uint32 *value)
{
	rtk_uint32 command;
	ret_t ret;

	ret = rtl8372n_setAsicRegBits(RTL8372N_PHY_DATA, 0xffff, port);
	if (ret != RT_ERR_OK)
		return ret;
	command = (page << 19) | (reg << 3) | RTL8372N_PHY_READ_CMD;
	ret = rtl8372n_setAsicReg(RTL8372N_PHY_CTRL, command);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_phy_wait_ready();
	if (ret != RT_ERR_OK)
		return ret;
	return rtl8372n_getAsicRegBits(RTL8372N_PHY_DATA_LOW, 0xffff,
					       value);
}

ret_t rtl8372n_phy_write(rtk_uint32 port_mask, rtk_uint32 page,
			  rtk_uint32 reg, rtk_uint32 value)
{
	rtk_uint32 command;
	ret_t ret;

	ret = rtl8372n_setAsicReg(RTL8372N_PHY_PORT_SELECT, port_mask);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicRegBits(RTL8372N_PHY_DATA, 0xffff, value);
	if (ret != RT_ERR_OK)
		return ret;
	command = (page << 19) | (reg << 3) | RTL8372N_PHY_WRITE_CMD;
	ret = rtl8372n_setAsicReg(RTL8372N_PHY_CTRL, command);
	if (ret != RT_ERR_OK)
		return ret;
	return rtl8372n_phy_wait_ready();
}

ret_t rtl8372n_phy_regbits_read(rtk_uint32 port, rtk_uint32 page,
				 rtk_uint32 reg, rtk_uint32 bits,
				 rtk_uint32 *value)
{
	rtk_uint32 data;
	ret_t ret;
	int shift;

	if (bits > 0xffff)
		return RT_ERR_INPUT;
	shift = rtl8372n_mask_shift(bits);
	if (shift < 0)
		return RT_ERR_INPUT;
	ret = rtl8372n_phy_read(port, page, reg, &data);
	if (ret != RT_ERR_OK)
		return ret;
	*value = (data & bits) >> shift;
	return RT_ERR_OK;
}

ret_t rtl8372n_phy_regbits_write(rtk_uint32 port_mask, rtk_uint32 page,
				  rtk_uint32 reg, rtk_uint32 bits,
				  rtk_uint32 value)
{
	rtk_uint32 data;
	ret_t ret;
	int port, shift;

	if (bits > 0xffff)
		return RT_ERR_INPUT;
	shift = rtl8372n_mask_shift(bits);
	if (shift < 0)
		return RT_ERR_INPUT;

	for (port = 0; port < 9; port++) {
		if (!(port_mask & (1U << port)))
			continue;
		ret = rtl8372n_phy_read(port, page, reg, &data);
		if (ret != RT_ERR_OK)
			return ret;
		data = (data & ~bits) | ((value << shift) & bits);
		ret = rtl8372n_phy_write(1U << port, page, reg, data);
		if (ret != RT_ERR_OK)
			return ret;
	}

	return RT_ERR_OK;
}

ret_t uc1_sram_write_8b(rtk_uint32 port, rtk_uint32 addr,
			 rtk_uint32 value)
{
	ret_t ret;

	ret = rtl8372n_phy_regbits_write(1U << port,
					 RTL8372N_PHY_VENDOR_PAGE, 0xa436,
					 0xffff, addr);
	if (ret != RT_ERR_OK)
		return ret;
	return rtl8372n_phy_regbits_write(1U << port,
					  RTL8372N_PHY_VENDOR_PAGE, 0xa438,
					  0xff00, value);
}

rtk_uint32 uc1_sram_read_8b(rtk_uint32 port, rtk_uint32 addr)
{
	rtk_uint32 value = 0;

	if (rtl8372n_phy_regbits_write(1U << port,
					RTL8372N_PHY_VENDOR_PAGE, 0xa436,
					0xffff, addr) != RT_ERR_OK)
		return 0;
	if (rtl8372n_phy_regbits_read(port, RTL8372N_PHY_VENDOR_PAGE,
				       0xa438, 0xff00, &value) != RT_ERR_OK)
		return 0;
	return value;
}

ret_t uc2_sram_write_8b(rtk_uint32 port, rtk_uint32 addr,
			 rtk_uint32 value)
{
	ret_t ret;

	ret = rtl8372n_phy_regbits_write(1U << port,
					 RTL8372N_PHY_VENDOR_PAGE, 0xb87c,
					 0xffff, addr);
	if (ret != RT_ERR_OK)
		return ret;
	return rtl8372n_phy_regbits_write(1U << port,
					  RTL8372N_PHY_VENDOR_PAGE, 0xb87e,
					  0xff00, value);
}

ret_t data_ram_write_8b(rtk_uint8 port, rtk_uint32 addr,
			 rtk_uint32 value)
{
	ret_t ret;
	rtk_uint32 bits = addr & 1 ? 0x00ff : 0xff00;

	ret = rtl8372n_phy_regbits_write(1U << port,
					 RTL8372N_PHY_VENDOR_PAGE, 0xb88e,
					 0xffff, addr);
	if (ret != RT_ERR_OK)
		return ret;
	return rtl8372n_phy_regbits_write(1U << port,
					  RTL8372N_PHY_VENDOR_PAGE, 0xb890,
					  bits, value);
}

static ret_t rtl8372n_sds_wait_ready(void)
{
	rtk_uint32 busy;
	ret_t ret;
	int retry;

	for (retry = 0; retry < RTL8372N_IO_RETRIES; retry++) {
		ret = rtl8372n_getAsicRegBit(RTL8372N_SDS_CTRL, 15, &busy);
		if (ret != RT_ERR_OK)
			return ret;
		if (!busy)
			return RT_ERR_OK;
	}

	return RT_ERR_BUSYWAIT_TIMEOUT;
}

ret_t rtl8372n_sds_reg_read(rtk_uint32 sds, rtk_uint32 reg,
			     rtk_uint32 page, rtk_uint32 *value)
{
	ret_t ret;

	ret = rtl8372n_sds_wait_ready();
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicRegBit(RTL8372N_SDS_CTRL, 0, sds);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicRegBits(RTL8372N_SDS_CTRL, 0x007e, reg);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicRegBits(RTL8372N_SDS_CTRL, 0x0f80, page);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicRegBit(RTL8372N_SDS_CTRL, 14, 0);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicRegBit(RTL8372N_SDS_CTRL, 15, 1);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_sds_wait_ready();
	if (ret != RT_ERR_OK)
		return ret;
	return rtl8372n_getAsicReg(RTL8372N_SDS_READ_DATA, value);
}

ret_t rtl8372n_sds_reg_write(rtk_uint32 sds, rtk_uint32 reg,
			      rtk_uint32 page, rtk_uint32 value)
{
	ret_t ret;

	ret = rtl8372n_sds_wait_ready();
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicReg(RTL8372N_SDS_WRITE_DATA, value);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicRegBit(RTL8372N_SDS_CTRL, 0, sds);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicRegBits(RTL8372N_SDS_CTRL, 0x007e, reg);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicRegBits(RTL8372N_SDS_CTRL, 0x0f80, page);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicRegBit(RTL8372N_SDS_CTRL, 14, 1);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicRegBit(RTL8372N_SDS_CTRL, 15, 1);
	if (ret != RT_ERR_OK)
		return ret;
	return rtl8372n_sds_wait_ready();
}

ret_t rtl8372n_sds_regbits_read(rtk_uint32 sds, rtk_uint32 reg,
				 rtk_uint32 page, rtk_uint32 bits,
				 rtk_uint32 *value)
{
	rtk_uint32 data;
	ret_t ret;
	int shift = rtl8372n_mask_shift(bits);

	if (shift < 0)
		return RT_ERR_INPUT;
	ret = rtl8372n_sds_reg_read(sds, reg, page, &data);
	if (ret != RT_ERR_OK)
		return ret;
	*value = (data & bits) >> shift;
	return RT_ERR_OK;
}

ret_t rtl8372n_sds_regbits_write(rtk_uint32 sds, rtk_uint32 reg,
				  rtk_uint32 page, rtk_uint32 bits,
				  rtk_uint32 value)
{
	rtk_uint32 data;
	ret_t ret;
	int shift = rtl8372n_mask_shift(bits);

	if (shift < 0)
		return RT_ERR_INPUT;
	ret = rtl8372n_sds_reg_read(sds, reg, page, &data);
	if (ret != RT_ERR_OK)
		return ret;
	data = (data & ~bits) | ((value << shift) & bits);
	return rtl8372n_sds_reg_write(sds, reg, page, data);
}

ret_t fw_reset_flow_tgr(rtk_uint32 sds)
{
	rtk_uint32 value;
	ret_t ret;

	ret = rtl8372n_sds_regbits_read(sds, 0x20, 0, 0x30, &value);
	if (ret != RT_ERR_OK || value == 1)
		return ret;
	ret = rtl8372n_sds_regbits_write(sds, 0x21, 0, 0x4, 1);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_sds_regbits_write(sds, 0x36, 5, 0x7800, 8);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_sds_reg_write(sds, 0x1f, 2, 0x1f);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_sds_reg_read(sds, 0x1f, 0x15, &value);
	if (ret != RT_ERR_OK)
		return ret;
	if (!((value & 0x40) | ((value & 0x80) == 0)))
		return RT_ERR_OK;
	ret = rtl8372n_sds_reg_read(sds, 5, 0, &value);
	if (ret != RT_ERR_OK)
		return ret;
	if (value & 1) {
		ret = rtl8372n_sds_reg_read(sds, 5, 0, &value);
		if (ret != RT_ERR_OK)
			return ret;
		if (!((value & 0x2) | ((value & 0x1000) == 0)))
			return RT_ERR_OK;
	}

	ret = rtl8372n_sds_regbits_write(sds, 0x20, 0, 0x30, 3);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_sds_regbits_write(sds, 0x20, 0, 0x30, 1);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_sds_regbits_write(sds, 0x20, 0, 0x30, 3);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_sds_regbits_write(sds, 0x20, 0, 0x30, 0);
	if (ret != RT_ERR_OK)
		return ret;

	if (!(value & 1))
		return rtl8372n_sds_reg_read(sds, 5, 0, &value);
	return RT_ERR_OK;
}
