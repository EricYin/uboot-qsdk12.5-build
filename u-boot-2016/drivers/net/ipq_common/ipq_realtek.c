// SPDX-License-Identifier: GPL-2.0-only
/* Xiaomi BE3600 Pro RTL8372N switch and RTL8221D PHY support. */

#include <common.h>
#include <asm-generic/errno.h>
#include <malloc.h>

#include "ipq_phy.h"
#include "ipq_realtek.h"
#include "rtl8372n/include/rtk_error.h"
#include "rtl8372n/include/rtl8372n_asicdrv.h"
#include "rtl8372n/include/rtl8372n_boot_io.h"
#include "rtl8372n/include/rtl8372n_switch.h"

#define RTL8372N_CHIP_ID_REG		0x0004
#define RTL8372N_CHIP_ID		0x83727000
#define RTL8372N_CPU_TAG_AWARE		0x603c
#define RTL8372N_CPU_TAG_CTRL		0x6720
#define RTL8372N_EXT_CPU_CTRL		0x6724
#define RTL8372N_VLAN_CTRL		0x4e14
#define RTL8372N_VLAN_INGRESS		0x4e18
#define RTL8372N_VLAN_EGRESS		0x6738
#define RTL8372N_MSTP_STATE0		0x5310
#define RTL8372N_PORT_ISOLATION(p)	(0x50c0 + ((p) * 4))
#define RTL8372N_PORT_FORCE(p)		(0x6344 + ((p) * 4))
#define RTL8372N_SDS_MODE_CTRL		0x7b20

#define RTL8372N_STOCK_CPU_TAG		0x00000500
#define RTL8372N_STOCK_EXT_CPU		0x0000000f
#define RTL8372N_HANDOFF_CPU_PORT	8
#define RTL8372N_HANDOFF_USER_MASK	0x000000f0
#define RTL8372N_HANDOFF_CPU_MASK	0x00000100
#define RTL8372N_STOCK_USER_FORCE	0x00000194
#define RTL8372N_STOCK_CPU_FORCE	0x000003a7
#define RTL8372N_STOCK_SDS_MODE		0x000009bf
#define REALTEK_DETECT_RETRIES		5
#define REALTEK_DETECT_DELAY_MS		20
#define REALTEK_STABLE_MISS_READS	2
#define REALTEK_SWITCH_CACHE_SIZE	2

#define MII_BMCR			0x00
#define MII_BMSR			0x01
#define MII_PHYSID1			0x02
#define MII_PHYSID2			0x03
#define MII_MMD_CTRL			0x0d
#define MII_MMD_DATA			0x0e
#define MII_PHYSR			0x1a
#define MII_PAGE_SELECT			0x1f
#define BMSR_LSTATUS			0x0004
#define RTL_PHYSR_DUPLEX		0x0008
#define RTL_PHYSR_SPEED_MASK		0x0630

struct rtl8372n_sds_patch {
	u8 page;
	u8 reg;
	u16 val;
};

static const struct rtl8372n_sds_patch rtl8372n_10g_an_patch[] = {
	{ 0x21, 0x10, 0x4480 }, { 0x21, 0x13, 0x0400 },
	{ 0x21, 0x18, 0x6d02 }, { 0x21, 0x1b, 0x424e },
	{ 0x21, 0x1d, 0x0002 }, { 0x36, 0x1c, 0x1390 },
	{ 0x36, 0x14, 0x003f }, { 0x36, 0x10, 0x0200 },
	{ 0x2e, 0x04, 0x0080 }, { 0x2e, 0x06, 0x0408 },
	{ 0x2e, 0x07, 0x020d }, { 0x2e, 0x09, 0x0601 },
	{ 0x2e, 0x0b, 0x222c }, { 0x2e, 0x0c, 0xa217 },
	{ 0x2e, 0x0d, 0xfe40 }, { 0x2e, 0x15, 0xf5c1 },
	{ 0x2e, 0x16, 0x0443 }, { 0x2e, 0x1d, 0xabb0 },
};

static const struct rtl8372n_sds_patch rtl8372n_10g_mac_patch[] = {
	{ 0x06, 0x12, 0x5078 }, { 0x07, 0x06, 0x9401 },
	{ 0x07, 0x08, 0x9401 }, { 0x07, 0x0a, 0x9401 },
	{ 0x07, 0x0c, 0x9401 }, { 0x1f, 0x0b, 0x0003 },
	{ 0x06, 0x03, 0xc45c }, { 0x06, 0x1f, 0x2100 },
};

static u32 rtl8372n_smi_addr;
static struct {
	u32 smi_addr;
	bool valid;
} rtl8372n_detect_cache[REALTEK_SWITCH_CACHE_SIZE];
static struct {
	u32 phy_addr;
	u16 id1;
	u16 id2;
	bool valid;
} rtl8221d_detect_cache;

extern int ipq_mdio_read(int mii_id, int regnum, ushort *data);
extern int ipq_mdio_write(int mii_id, int regnum, u16 value);

unsigned int rtl837x_mdio_lock(void)
{
	return 0;
}

unsigned int rtl837x_mdio_unlock(void)
{
	return 0;
}

unsigned int rtl837x_mdio_read(unsigned int reg, unsigned int *val)
{
	int ret = ipq_mdio_read(rtl8372n_smi_addr, reg, NULL);

	if (ret < 0)
		return ret;
	*val = ret;
	return 0;
}

unsigned int rtl837x_mdio_write(unsigned int reg, unsigned int val)
{
	return ipq_mdio_write(rtl8372n_smi_addr, reg, val);
}

static bool realtek_detect_stable_nonmatch(u32 sample, bool valid,
					    u32 *last_nonmatch,
					    int *stable_misses)
{
	if (!valid) {
		*last_nonmatch = 0;
		*stable_misses = 0;
		return false;
	}

	if (sample == *last_nonmatch) {
		(*stable_misses)++;
	} else {
		*last_nonmatch = sample;
		*stable_misses = 1;
	}

	return *stable_misses >= REALTEK_STABLE_MISS_READS;
}

static void rtl8372n_cache_detection(u32 smi_addr)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(rtl8372n_detect_cache); i++) {
		if (rtl8372n_detect_cache[i].valid &&
		    rtl8372n_detect_cache[i].smi_addr == smi_addr)
			return;
		if (!rtl8372n_detect_cache[i].valid) {
			rtl8372n_detect_cache[i].smi_addr = smi_addr;
			rtl8372n_detect_cache[i].valid = true;
			return;
		}
	}
}

static bool rtl8372n_detection_cached(u32 smi_addr)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(rtl8372n_detect_cache); i++)
		if (rtl8372n_detect_cache[i].valid &&
		    rtl8372n_detect_cache[i].smi_addr == smi_addr)
			return true;

	return false;
}

static void rtl8372n_invalidate_detection(u32 smi_addr)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(rtl8372n_detect_cache); i++)
		if (rtl8372n_detect_cache[i].valid &&
		    rtl8372n_detect_cache[i].smi_addr == smi_addr) {
			rtl8372n_detect_cache[i].valid = false;
			return;
		}
}

int ipq_rtl8372n_switch_detect(u32 smi_addr)
{
	u32 chip = 0;
	u32 last_nonmatch = 0;
	ret_t ret = RT_ERR_FAILED;
	int attempt, stable_misses = 0;

	rtl8372n_smi_addr = smi_addr;
	for (attempt = 0; attempt < REALTEK_DETECT_RETRIES; attempt++) {
		ret = rtl8372n_getAsicReg(RTL8372N_CHIP_ID_REG, &chip);
		if (ret == RT_ERR_OK && chip == RTL8372N_CHIP_ID) {
			rtl8372n_cache_detection(smi_addr);
			printf("RTL8372N@%x detected chip=0x%08x\n",
			       smi_addr, chip);
			return 0;
		}
		if (realtek_detect_stable_nonmatch(
				chip,
				ret == RT_ERR_OK && chip != 0 && chip != ~0U,
				&last_nonmatch, &stable_misses)) {
			printf("RTL8372N@%x not detected stable chip=0x%08x samples=%d\n",
			       smi_addr, chip, stable_misses);
			return -ENODEV;
		}
		if (attempt + 1 < REALTEK_DETECT_RETRIES)
			mdelay(REALTEK_DETECT_DELAY_MS);
	}

	printf("RTL8372N@%x not detected ret=%d chip=0x%08x\n",
	       smi_addr, ret, chip);
	return -ENODEV;
}

int ipq_rtl8221d_phy_detect(u32 phy_addr)
{
	u32 sample = 0, last_nonmatch = 0;
	int attempt, id1 = -1, id2 = -1, stable_misses = 0;
	bool valid;

	for (attempt = 0; attempt < REALTEK_DETECT_RETRIES; attempt++) {
		id1 = ipq_mdio_read(phy_addr, MII_PHYSID1, NULL);
		id2 = ipq_mdio_read(phy_addr, MII_PHYSID2, NULL);
		if (id1 == 0x001c && id2 == 0xc849) {
			rtl8221d_detect_cache.phy_addr = phy_addr;
			rtl8221d_detect_cache.id1 = id1;
			rtl8221d_detect_cache.id2 = id2;
			rtl8221d_detect_cache.valid = true;
			printf("RTL8221D@%x detected id=0x%04x:0x%04x\n",
			       phy_addr, id1, id2);
			return 0;
		}
		valid = id1 > 0 && id1 < 0xffff &&
			id2 > 0 && id2 < 0xffff;
		if (valid)
			sample = ((u32)id1 << 16) | (u16)id2;
		if (realtek_detect_stable_nonmatch(
				sample, valid, &last_nonmatch,
				&stable_misses)) {
			printf("RTL8221D@%x not detected stable id=0x%04x:0x%04x samples=%d\n",
			       phy_addr, id1, id2, stable_misses);
			return -ENODEV;
		}
		if (attempt + 1 < REALTEK_DETECT_RETRIES)
			mdelay(REALTEK_DETECT_DELAY_MS);
	}

	printf("RTL8221D@%x not detected id=0x%04x:0x%04x\n",
	       phy_addr, id1, id2);
	return -ENODEV;
}

static int rtl8372n_sds_apply_patch(
		const struct rtl8372n_sds_patch *patch, size_t count)
{
	size_t i;
	ret_t ret;

	for (i = 0; i < count; i++) {
		ret = rtl8372n_sds_reg_write(1, patch[i].reg,
					      patch[i].page, patch[i].val);
		if (ret != RT_ERR_OK)
			return ret;
	}

	return RT_ERR_OK;
}

static int rtl8372n_sds_power_down(void)
{
	static const struct {
		u16 mask;
		u8 val;
		u16 delay_us;
	} steps[] = {
		{ 0x0030, 3, 10 }, { 0x0030, 1, 100 },
		{ 0x00c0, 1, 10 }, { 0x00c0, 3, 100 },
		{ 0x0c00, 3, 10 }, { 0x0c00, 1, 100 },
	};
	size_t i;
	ret_t ret;

	for (i = 0; i < ARRAY_SIZE(steps); i++) {
		ret = rtl8372n_sds_regbits_write(1, 0x20, 0,
						  steps[i].mask, steps[i].val);
		if (ret != RT_ERR_OK)
			return ret;
		udelay(steps[i].delay_us);
	}

	return RT_ERR_OK;
}

static int rtl8372n_sds_mode_toggle(void)
{
	static const struct {
		u16 mask;
		u8 val;
		u16 delay_us;
	} steps[] = {
		{ 0x0030, 3, 10 }, { 0x0030, 1, 100 },
		{ 0x00c0, 1, 10 }, { 0x00c0, 3, 100 },
		{ 0x0c00, 3, 10 }, { 0x0c00, 1, 10 },
		{ 0x0c00, 1, 10 }, { 0x0c00, 3, 100 },
		{ 0x0c00, 0, 10 }, { 0x00c0, 3, 10 },
		{ 0x00c0, 1, 100 }, { 0x00c0, 0, 10 },
		{ 0x0030, 1, 10 }, { 0x0030, 3, 100 },
		{ 0x0030, 0, 100 },
	};
	size_t i;
	ret_t ret;

	for (i = 0; i < ARRAY_SIZE(steps); i++) {
		ret = rtl8372n_sds_regbits_write(1, 0x20, 0,
						  steps[i].mask, steps[i].val);
		if (ret != RT_ERR_OK)
			return ret;
		udelay(steps[i].delay_us);
	}

	ret = rtl8372n_sds_reg_write(1, 0x1f, 0, 0x000b);
	if (ret != RT_ERR_OK)
		return ret;
	udelay(100);

	ret = rtl8372n_sds_reg_write(1, 0x1f, 0, 0x0000);
	udelay(100);
	return ret;
}

static int rtl8372n_set_handoff_isolation(const char *stage)
{
	ret_t ret;
	u32 readback;
	int port;

	/* Keep WAN and user ports on the switch behind the CPU port during
	 * U-Boot handoff.  The Linux DSA driver programs the same topology.
	 */
	for (port = 4; port <= 7; port++) {
		ret = rtl8372n_setAsicReg(RTL8372N_PORT_ISOLATION(port),
					  RTL8372N_HANDOFF_CPU_MASK);
		if (ret != RT_ERR_OK)
			return ret;
	}
	ret = rtl8372n_setAsicReg(RTL8372N_PORT_ISOLATION(
					 RTL8372N_HANDOFF_CPU_PORT),
				  RTL8372N_HANDOFF_USER_MASK);
	if (ret != RT_ERR_OK)
		return ret;

	for (port = 4; port <= RTL8372N_HANDOFF_CPU_PORT; port++) {
		u32 expected = port == RTL8372N_HANDOFF_CPU_PORT ?
				RTL8372N_HANDOFF_USER_MASK :
				RTL8372N_HANDOFF_CPU_MASK;

		ret = rtl8372n_getAsicReg(RTL8372N_PORT_ISOLATION(port),
					  &readback);
		if (ret != RT_ERR_OK)
			return ret;
		if (readback != expected) {
			printf("RTL8372N@%x %s iso p%d=0x%x want=0x%x\n",
			       rtl8372n_smi_addr, stage, port,
			       readback, expected);
			return RT_ERR_FAILED;
		}
	}

	printf("RTL8372N@%x %s isolation user->cpu=0x%03x cpu->users=0x%03x\n",
	       rtl8372n_smi_addr, stage, RTL8372N_HANDOFF_CPU_MASK,
	       RTL8372N_HANDOFF_USER_MASK);
	return RT_ERR_OK;
}

static int rtl8372n_stock_handoff_init(void)
{
	ret_t ret;
	u32 chip, mode, force, tag;
	int port;

	ret = fw_reset_flow_tgr(1);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_set_handoff_isolation("early");
	if (ret != RT_ERR_OK)
		return ret;

	ret = rtl8372n_setAsicReg(RTL8372N_PORT_FORCE(8), 1);
	if (ret != RT_ERR_OK)
		return ret;
	mdelay(1);
	ret = rtl8372n_setAsicReg(RTL8372N_PORT_FORCE(8),
				  RTL8372N_STOCK_CPU_FORCE);
	if (ret != RT_ERR_OK)
		return ret;

	ret = rtl8372n_sds_power_down();
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicReg(RTL8372N_SDS_MODE_CTRL,
				  RTL8372N_STOCK_SDS_MODE);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_sds_apply_patch(rtl8372n_10g_an_patch,
				       ARRAY_SIZE(rtl8372n_10g_an_patch));
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_sds_apply_patch(rtl8372n_10g_mac_patch,
				       ARRAY_SIZE(rtl8372n_10g_mac_patch));
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_sds_regbits_write(1, 7, 0x11, 0x000f, 0x000f);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_sds_mode_toggle();
	if (ret != RT_ERR_OK)
		return ret;
	ret = fw_reset_flow_tgr(1);
	if (ret != RT_ERR_OK)
		return ret;

	for (port = 4; port <= 7; port++) {
		ret = rtl8372n_setAsicReg(RTL8372N_PORT_FORCE(port),
					  RTL8372N_STOCK_USER_FORCE);
		if (ret != RT_ERR_OK)
			return ret;
	}
	ret = rtl8372n_setAsicReg(RTL8372N_PORT_FORCE(8),
				  RTL8372N_STOCK_CPU_FORCE);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_set_handoff_isolation("final");
	if (ret != RT_ERR_OK)
		return ret;

	ret = rtl8372n_setAsicReg(RTL8372N_CPU_TAG_CTRL,
				  RTL8372N_STOCK_CPU_TAG);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicReg(RTL8372N_EXT_CPU_CTRL,
				  RTL8372N_STOCK_EXT_CPU);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicReg(RTL8372N_CPU_TAG_AWARE, 0);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicReg(RTL8372N_VLAN_CTRL, 0);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicReg(RTL8372N_VLAN_INGRESS, 0);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicReg(RTL8372N_VLAN_EGRESS, 0x000fffff);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_setAsicReg(RTL8372N_MSTP_STATE0, 0x000fffff);
	if (ret != RT_ERR_OK)
		return ret;

	ret = rtl8372n_getAsicReg(RTL8372N_CHIP_ID_REG, &chip);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_getAsicReg(RTL8372N_SDS_MODE_CTRL, &mode);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_getAsicReg(RTL8372N_PORT_FORCE(8), &force);
	if (ret != RT_ERR_OK)
		return ret;
	ret = rtl8372n_getAsicReg(RTL8372N_CPU_TAG_CTRL, &tag);
	if (ret != RT_ERR_OK)
		return ret;

	printf("RTL8372N@%x handoff chip=0x%08x sds=0x%08x force=0x%08x tag=0x%08x\n",
	       rtl8372n_smi_addr, chip, mode, force, tag);
	if (chip != RTL8372N_CHIP_ID || mode != RTL8372N_STOCK_SDS_MODE ||
	    force != RTL8372N_STOCK_CPU_FORCE ||
	    tag != RTL8372N_STOCK_CPU_TAG)
		return -EIO;

	return 0;
}

static u8 rtl8372n_get_link_status(u32 dev_id, u32 phy_id)
{
	return 0;
}

static u32 rtl8372n_get_duplex(u32 dev_id, u32 phy_id,
			       fal_port_duplex_t *duplex)
{
	*duplex = FAL_FULL_DUPLEX;
	return 0;
}

static u32 rtl8372n_get_speed(u32 dev_id, u32 phy_id,
			      fal_port_speed_t *speed)
{
	*speed = FAL_SPEED_10000;
	return 0;
}

int ipq_rtl8372n_switch_init(struct phy_ops **ops, u32 smi_addr)
{
	struct phy_ops *rtl_ops;
	int ret;

	rtl_ops = malloc(sizeof(*rtl_ops));
	if (!rtl_ops)
		return -ENOMEM;
	rtl_ops->phy_get_link_status = rtl8372n_get_link_status;
	rtl_ops->phy_get_speed = rtl8372n_get_speed;
	rtl_ops->phy_get_duplex = rtl8372n_get_duplex;

	rtl8372n_smi_addr = smi_addr;
	rtk_switch_initialState_set(INIT_NOT_COMPLETED);
	printf("RTL8372N@%x cold init begin\n", smi_addr);
	ret = rtl8372n_set_handoff_isolation("pre-init");
	if (ret != RT_ERR_OK)
		printf("RTL8372N@%x pre-init isolation unavailable: %d; retry after init\n",
		       smi_addr, ret);
	if (rtl8372n_detection_cached(smi_addr))
		ret = rtk_switch_init_by_chip(CHIP_RTL8372N);
	else
		ret = rtk_switch_init();
	if (ret == RT_ERR_OK)
		ret = rtl8372n_stock_handoff_init();
	if (ret) {
		printf("RTL8372N@%x cold init failed: %d\n", smi_addr, ret);
		rtl8372n_invalidate_detection(smi_addr);
		free(rtl_ops);
		return -EIO;
	}

	*ops = rtl_ops;
	printf("RTL8372N@%x cold init complete\n", smi_addr);
	return 0;
}

static int rtl8221d_mmd_write(u32 phy, u16 devad, u16 reg, u16 val)
{
	int ret;

	ret = ipq_mdio_write(phy, MII_MMD_CTRL, devad);
	if (ret)
		return ret;
	ret = ipq_mdio_write(phy, MII_MMD_DATA, reg);
	if (ret)
		return ret;
	ret = ipq_mdio_write(phy, MII_MMD_CTRL, devad | 0x4000);
	if (ret)
		return ret;
	return ipq_mdio_write(phy, MII_MMD_DATA, val);
}

static int rtl8221d_mmd_read(u32 phy, u16 devad, u16 reg)
{
	int ret;

	ret = ipq_mdio_write(phy, MII_MMD_CTRL, devad);
	if (ret)
		return ret;
	ret = ipq_mdio_write(phy, MII_MMD_DATA, reg);
	if (ret)
		return ret;
	ret = ipq_mdio_write(phy, MII_MMD_CTRL, devad | 0x4000);
	if (ret)
		return ret;
	return ipq_mdio_read(phy, MII_MMD_DATA, NULL);
}

static int rtl8221d_vend2_write(u32 phy, u16 reg, u16 val)
{
	int oldpage, ret;
	u16 page = reg >> 4;
	u16 page_reg = 16 + ((reg & 0xf) >> 1);

	oldpage = ipq_mdio_read(phy, MII_PAGE_SELECT, NULL);
	if (oldpage < 0)
		return oldpage;
	ret = ipq_mdio_write(phy, MII_PAGE_SELECT, page);
	if (ret)
		return ret;
	ret = ipq_mdio_write(phy, page_reg, val);
	if (ipq_mdio_write(phy, MII_PAGE_SELECT, oldpage) && !ret)
		ret = -EIO;
	return ret;
}

static u8 rtl8221d_get_link_status(u32 dev_id, u32 phy_id)
{
	int status;

	status = ipq_mdio_read(phy_id, MII_BMSR, NULL);
	status = ipq_mdio_read(phy_id, MII_BMSR, NULL);
	return status >= 0 && (status & BMSR_LSTATUS) ? 0 : 1;
}

static u32 rtl8221d_get_duplex(u32 dev_id, u32 phy_id,
			       fal_port_duplex_t *duplex)
{
	int status = ipq_mdio_read(phy_id, MII_PHYSR, NULL);

	if (status < 0)
		return status;
	*duplex = status & RTL_PHYSR_DUPLEX ?
		  FAL_FULL_DUPLEX : FAL_HALF_DUPLEX;
	return 0;
}

static u32 rtl8221d_get_speed(u32 dev_id, u32 phy_id,
			      fal_port_speed_t *speed)
{
	int status = ipq_mdio_read(phy_id, MII_PHYSR, NULL);

	if (status < 0)
		return status;
	switch (status & RTL_PHYSR_SPEED_MASK) {
	case 0x0000:
		*speed = FAL_SPEED_10;
		break;
	case 0x0010:
		*speed = FAL_SPEED_100;
		break;
	case 0x0020:
		*speed = FAL_SPEED_1000;
		break;
	case 0x0200:
		*speed = FAL_SPEED_10000;
		break;
	case 0x0210:
		*speed = FAL_SPEED_2500;
		break;
	case 0x0220:
		*speed = FAL_SPEED_5000;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

int ipq_rtl8221d_phy_init(struct phy_ops **ops, u32 phy_addr)
{
	static const struct {
		u16 devad;
		u16 reg;
		u16 val;
	} init_seq[] = {
		{ 30, 0x75f3, 0x0000 },
		{ 30, 0x697a, 0x0001 },
		{ 30, 0x6a04, 0x0503 },
		{ 30, 0x6f10, 0xd455 },
		{ 30, 0x6f11, 0x8020 },
		{ 7, 0x003c, 0x0000 },
		{ 7, 0x003e, 0x0000 },
		{ 7, MII_BMCR, 0x3200 },
	};
	struct phy_ops *rtl_ops;
	int id1, id2, option, ctrl3, ret;
	size_t i;

	rtl_ops = malloc(sizeof(*rtl_ops));
	if (!rtl_ops)
		return -ENOMEM;
	rtl_ops->phy_get_link_status = rtl8221d_get_link_status;
	rtl_ops->phy_get_speed = rtl8221d_get_speed;
	rtl_ops->phy_get_duplex = rtl8221d_get_duplex;

	if (rtl8221d_detect_cache.valid &&
	    rtl8221d_detect_cache.phy_addr == phy_addr) {
		id1 = rtl8221d_detect_cache.id1;
		id2 = rtl8221d_detect_cache.id2;
	} else {
		id1 = ipq_mdio_read(phy_addr, MII_PHYSID1, NULL);
		id2 = ipq_mdio_read(phy_addr, MII_PHYSID2, NULL);
		printf("RTL8221D@%x PHY ID1/ID2: 0x%04x/0x%04x\n",
		       phy_addr, id1, id2);
	}
	if (id1 != 0x001c || id2 != 0xc849) {
		free(rtl_ops);
		return -ENODEV;
	}

	for (i = 0; i < ARRAY_SIZE(init_seq); i++) {
		ret = rtl8221d_mmd_write(phy_addr, init_seq[i].devad,
					 init_seq[i].reg, init_seq[i].val);
		if (ret)
			goto fail;
	}
	ret = rtl8221d_vend2_write(phy_addr, 0xd032, 0x0027);
	if (ret)
		goto fail;

	option = rtl8221d_mmd_read(phy_addr, 30, 0x697a);
	ctrl3 = rtl8221d_mmd_read(phy_addr, 30, 0x7580);
	printf("RTL8221D@%x stock init complete option=0x%04x ctrl3=0x%04x\n",
	       phy_addr, option, ctrl3);
	*ops = rtl_ops;
	return 0;

fail:
	printf("RTL8221D@%x stock init failed: %d\n", phy_addr, ret);
	if (rtl8221d_detect_cache.phy_addr == phy_addr)
		rtl8221d_detect_cache.valid = false;
	free(rtl_ops);
	return ret;
}
