// SPDX-License-Identifier: GPL-2.0-only

#include "include/rtk_error.h"
#include "include/rtl8372n_types.h"
#include "include/rtl8372n_smi.h"

#define RTL8372N_SMI_CTRL	0x15
#define RTL8372N_SMI_BUSY	0x4
#define RTL8372N_SMI_RETRIES	1000

extern unsigned int rtl837x_mdio_lock(void);
extern unsigned int rtl837x_mdio_unlock(void);
extern unsigned int rtl837x_mdio_read(unsigned int reg, unsigned int *data);
extern unsigned int rtl837x_mdio_write(unsigned int reg, unsigned int data);

static rtk_int32 rtl8372n_smi_wait_ready(void)
{
	rtk_uint32 busy;
	int retry;

	for (retry = 0; retry < RTL8372N_SMI_RETRIES; retry++) {
		if (rtl837x_mdio_read(RTL8372N_SMI_CTRL, &busy))
			return RT_ERR_SMI;
		if (!(busy & RTL8372N_SMI_BUSY))
			return RT_ERR_OK;
		udelay(10);
	}

	return RT_ERR_BUSYWAIT_TIMEOUT;
}

rtk_int32 smi_read(rtk_uint32 addr, rtk_uint32 *data)
{
	rtk_uint32 lo, hi;
	rtk_int32 ret;

	if (addr > 0xffff)
		return RT_ERR_INPUT;
	if (!data)
		return RT_ERR_NULL_POINTER;

	rtl837x_mdio_lock();
	ret = rtl8372n_smi_wait_ready();
	if (ret != RT_ERR_OK)
		goto out;
	if (rtl837x_mdio_write(MDC_MDIO_ADDRESS_REG, addr) ||
	    rtl837x_mdio_write(RTL8372N_SMI_CTRL, MDC_MDIO_READ_OP)) {
		ret = RT_ERR_SMI;
		goto out;
	}
	ret = rtl8372n_smi_wait_ready();
	if (ret != RT_ERR_OK)
		goto out;
	if (rtl837x_mdio_read(MDC_MDIO_DATAL_REG, &lo) ||
	    rtl837x_mdio_read(MDC_MDIO_DATAH_REG, &hi)) {
		ret = RT_ERR_SMI;
		goto out;
	}

	*data = lo | (hi << 16);
out:
	rtl837x_mdio_unlock();
	return ret;
}

rtk_int32 smi_write(rtk_uint32 addr, rtk_uint32 data)
{
	rtk_int32 ret;

	if (addr > 0xffff)
		return RT_ERR_INPUT;

	rtl837x_mdio_lock();
	ret = rtl8372n_smi_wait_ready();
	if (ret != RT_ERR_OK)
		goto out;
	if (rtl837x_mdio_write(MDC_MDIO_ADDRESS_REG, addr) ||
	    rtl837x_mdio_write(MDC_MDIO_DATAL_REG, data & 0xffff) ||
	    rtl837x_mdio_write(MDC_MDIO_DATAH_REG, data >> 16) ||
	    rtl837x_mdio_write(RTL8372N_SMI_CTRL, MDC_MDIO_WRITE_OP)) {
		ret = RT_ERR_SMI;
		goto out;
	}
	ret = rtl8372n_smi_wait_ready();
out:
	rtl837x_mdio_unlock();
	return ret;
}
