/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPQ_REALTEK_H_
#define _IPQ_REALTEK_H_

#include "ipq_phy.h"

int ipq_rtl8372n_switch_init(struct phy_ops **ops, u32 smi_addr);
int ipq_rtl8372n_switch_detect(u32 smi_addr);
int ipq_rtl8221d_phy_init(struct phy_ops **ops, u32 phy_addr);
int ipq_rtl8221d_phy_detect(u32 phy_addr);

#endif
