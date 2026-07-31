#include "include/rtk_error.h"
#include "include/rtl8372n_switch.h"
#include "include/rtl8372n_asicdrv.h"
#include "include/rtl8372n_boot_io.h"

#include <common.h>

#define msleep(ms) mdelay(ms)

static init_state_t init_state = INIT_NOT_COMPLETED;


struct patch_entry16_4 {
    rtk_uint16 reg_addr;   // 要修改的寄存器地址
    rtk_uint16 end_bit;    // 位域结束位置 (高位)
    rtk_uint16 start_bit;  // 位域起始位置 (低位)
    rtk_uint16 value;      // 要写入的值
};





static const struct patch_entry16_4 RTCT_para_6818C_231206_patch[] =
{
    {0xa436,0xf,0x0,0x81a3},{0xa436,0xf,0x0,0x81a3},{0xa438,0xf,0x8,0x32},{0xa436,0xf,0x0,0x81a4},{0xa438,0xf,0x8,0xc0},{0xa436,0xf,0x0,0x81a5},{0xa438,0xf,0x8,0x32},{0xa436,0xf,0x0,0x81a8},{0xa438,0xf,0x8,0x1d},{0xa436,0xf,0x0,0x81af},{0xa438,0xf,0x8,0x25},{0xa436,0xf,0x0,0x81b2},{0xa438,0xf,0x8,0x9},{0xa436,0xf,0x0,0x81b6},{0xa438,0xf,0x8,0x3f},{0xa436,0xf,0x0,0x81b7},{0xa438,0xf,0x8,0x48},{0xa436,0xf,0x0,0x81b8},{0xa438,0xf,0x8,0xc},{0xa436,0xf,0x0,0x81b9},{0xa438,0xf,0x8,0x4},{0xa436,0xf,0x0,0x81ba},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x81bb},{0xa438,0xf,0x8,0x20},{0xa436,0xf,0x0,0x81bc},{0xa438,0xf,0x8,0x4},{0xa436,0xf,0x0,0x81bd},{0xa438,0xf,0x8,0x20},{0xa436,0xf,0x0,0x81be},{0xa438,0xf,0x8,0x1a},{0xa436,0xf,0x0,0x81bf},{0xa438,0xf,0x8,0xe0},{0xa436,0xf,0x0,0x81c0},{0xa438,0xf,0x8,0x1},{0xa436,0xf,0x0,0x81c1},{0xa438,0xf,0x8,0x3a},{0xa436,0xf,0x0,0x81c2},{0xa438,0xf,0x8,0x1c},{0xa436,0xf,0x0,0x81c3},{0xa438,0xf,0x8,0x60},{0xa436,0xf,0x0,0x81c4},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x81c5},{0xa438,0xf,0x8,0x11},{0xa436,0xf,0x0,0x81c6},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x81c7},{0xa438,0xf,0x8,0xcf},{0xa436,0xf,0x0,0x81c8},{0xa438,0xf,0x8,0xff},{0xa436,0xf,0x0,0x81c9},{0xa438,0xf,0x8,0xb0},{0xa436,0xf,0x0,0x81ca},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x81cb},{0xa438,0xf,0x8,0xf},{0xa436,0xf,0x0,0x81cc},{0xa438,0xf,0x8,0x11},{0xa436,0xf,0x0,0x81cd},{0xa438,0xf,0x8,0xc0},{0xa436,0xf,0x0,0x81ce},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x81cf},{0xa438,0xf,0x8,0xe},{0xa436,0xf,0x0,0x81d0},{0xa438,0xf,0x8,0xff},{0xa436,0xf,0x0,0x81d1},{0xa438,0xf,0x8,0xbe},{0xa436,0xf,0x0,0x81d2},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x81d3},{0xa438,0xf,0x8,0x18},{0xa436,0xf,0x0,0x81d4},{0xa438,0xf,0x8,0x8},{0xa436,0xf,0x0,0x81d5},{0xa438,0xf,0x8,0x70},{0xa436,0xf,0x0,0x81d6},{0xa438,0xf,0x8,0xff},{0xa436,0xf,0x0,0x81d7},{0xa438,0xf,0x8,0x37},{0xa436,0xf,0x0,0x81d8},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x81d9},{0xa438,0xf,0x8,0x48},{0xa436,0xf,0x0,0x81da},{0xa438,0xf,0x8,0xff},{0xa436,0xf,0x0,0x81db},{0xa438,0xf,0x8,0xf4},{0xa436,0xf,0x0,0x81dc},{0xa438,0xf,0x8,0xeb},{0xa436,0xf,0x0,0x81dd},{0xa438,0xf,0x8,0xa0},{0xa436,0xf,0x0,0x81de},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x81df},{0xa438,0xf,0x8,0x2b},{0xa436,0xf,0x0,0x81e0},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x81e1},{0xa438,0xf,0x8,0x9c},{0xa436,0xf,0x0,0x81e2},{0xa438,0xf,0x8,0xff},{0xa436,0xf,0x0,0x81e3},{0xa438,0xf,0x8,0xb2},{0xa436,0xf,0x0,0x81e4},{0xa438,0xf,0x8,0xeb},{0xa436,0xf,0x0,0x81e5},{0xa438,0xf,0x8,0xaf},{0xa436,0xf,0x0,0x81e6},{0xa438,0xf,0x8,0x2},{0xa436,0xf,0x0,0x81e7},{0xa438,0xf,0x8,0x92},{0xa436,0xf,0x0,0x81e8},{0xa438,0xf,0x8,0xfe},{0xa436,0xf,0x0,0x81e9},{0xa438,0xf,0x8,0xe4},{0xa436,0xf,0x0,0x81ea},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x81eb},{0xa438,0xf,0x8,0x3c},{0xa436,0xf,0x0,0x81ec},{0xa438,0xf,0x8,0x6},{0xa436,0xf,0x0,0x81ed},{0xa438,0xf,0x8,0xf2},{0xa436,0xf,0x0,0x81ee},{0xa438,0xf,0x8,0xff},{0xa436,0xf,0x0,0x81ef},{0xa438,0xf,0x8,0x3a},{0xa436,0xf,0x0,0x81f0},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x81f1},{0xa438,0xf,0x8,0x8a},{0xa436,0xf,0x0,0x81f2},{0xa438,0xf,0x8,0xff},{0xa436,0xf,0x0,0x81f3},{0xa438,0xf,0x8,0xd0},{0xa436,0xf,0x0,0x81f4},{0xa438,0xf,0x8,0xa},{0xa436,0xf,0x0,0x81f5},{0xa438,0xf,0x8,0x9},{0xa436,0xf,0x0,0x81f6},{0xa438,0xf,0x8,0xff},{0xa436,0xf,0x0,0x81f7},{0xa438,0xf,0x8,0xdc},{0xa436,0xf,0x0,0x81f8},{0xa438,0xf,0x8,0xff},{0xa436,0xf,0x0,0x81f9},{0xa438,0xf,0x8,0x30},{0xa436,0xf,0x0,0x81fa},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x8700},{0xa438,0xf,0x8,0x1},{0xa436,0xf,0x0,0x8701},{0xa438,0xf,0x8,0x4},{0xa436,0xf,0x0,0x8018},{0xa438,0xf,0x8,0x70},{0xa436,0xf,0x0,0x81a6},{0xa438,0xf,0x8,0xc0},{0xa436,0xf,0x0,0x81a9},{0xa438,0xf,0x8,0x0},{0xa436,0xf,0x0,0x81b0},{0xa438,0xf,0x8,0x5},{0xa436,0xf,0x0,0x81b3},{0xa438,0xf,0x8,0x1d},{0xa436,0xf,0x0,0x81fb},{0xa438,0xf,0x8,0x6c},{0xa436,0xf,0x0,0x8702},{0xa438,0xf,0x8,0x50},{0xFFFF,0xFFFF,0xFFFF,0xFFFF},
};





ret_t RTCT_para_6818C_231206(rtk_uint32 port_mask)
{

    ret_t result;
    const struct patch_entry16_4 *patch_data = RTCT_para_6818C_231206_patch;

    for (int port_index = 0; port_index != 8; ++port_index )
    {
        // 检查端口是否在端口掩码内
        while ( ((1 << port_index) & port_mask) == 0 )
        {
            if ( ++port_index == 8 ) return result;
        }
        rtk_uint32 patch_index = 0;
        // 应用所有补丁条目
        while(1){
            if (patch_data[patch_index].reg_addr == 0xFFFF && patch_data[patch_index].value == 0xFFFF) {
                break;
            }
            rtk_uint32 bit_offset = patch_data[patch_index].start_bit;
            rtk_uint32 bit_width = patch_data[patch_index].end_bit -patch_data[patch_index].start_bit + 1;
            rtk_uint32 bit_mask = 1 << bit_offset;
            if(bit_width != 1){
                bit_mask = ((1 << bit_width) - 1) << bit_offset;
            }

            result = rtl8372n_phy_regbits_write(1 << port_index, 31, patch_data[patch_index].reg_addr, bit_mask, patch_data[patch_index].value);
            if (result != RT_ERR_OK) return result;
            patch_index += 1; // 移动到下一个补丁条目
        }
    }
    return result;
}

ret_t data_ram_patch_6818C_221026(rtk_uint32 port_mask)
{
    ret_t result = port_mask;                          // 函数返回值

    // 定义关键寄存器地址
    const rtk_uint32 DATA_RAM_CTRL_REG = 0xB876; // 47254 (0xB876)
    const rtk_uint32 DATA_RAM_ADDR_REG = 0xB872; // 47250 (0xB872)

    // 定义全局变量地址
    const rtk_uint32 DATA_RAM_ADDRESS = 0xC206;
    const rtk_uint32 DATA_RAM_VALUE = 0xB1;

    // 遍历所有端口 (0-7)
    for (rtk_uint32 port_index = 0; port_index < 8; port_index++) {
        // 3.1 检查端口是否在掩码中
        if (!(port_mask & (1 << port_index))) continue;

        rtk_uint32 port_bit = 1 << port_index; // 端口位掩码

        // 3.2 禁用数据RAM访问 (47254[0]=0)
        rtl8372n_phy_regbits_write(port_bit, 31, DATA_RAM_CTRL_REG, 1, 0);

        // 3.3 清理地址寄存器高8位 (47250[15:8]=0)
        rtl8372n_phy_regbits_write(port_bit, 31, DATA_RAM_ADDR_REG, 0xFF00, 0);

        // 3.4 写入数据RAM
        data_ram_write_8b(port_index, DATA_RAM_ADDRESS, DATA_RAM_VALUE);

        // 3.5 启用数据RAM访问 (47254[0]=1)
        result = rtl8372n_phy_regbits_write(port_bit, 31, DATA_RAM_CTRL_REG, 1, 1);
    }

    return result;
}

ret_t afe_patch_6818C_220607(rtk_uint16 port_mask)
{
    // 步骤1: 定义关键寄存器地址
    const rtk_uint32 AFE_REG1 = 0xBF84; // 49028 (0xBF84)
    const rtk_uint32 AFE_REG2 = 0xBF8C; // 49036 (0xBF8C)

    // 步骤2: 定义位掩码和值
    const rtk_uint32 REG1_MASK = 0x7;   // 位掩码 (低3位)
    const rtk_uint32 REG1_VALUE = 4;    // 要写入的值
    const rtk_uint32 REG2_MASK = 0x7C0; // 位掩码 (位[10:6])
    const rtk_uint32 REG2_VALUE = 0;    // 要写入的值

    // 步骤3: 遍历所有端口 (0-7)
    for (int port_index = 0; port_index < 8; port_index++) {
        // 3.1 检查端口是否在掩码中
        if (!(port_mask & (1 << port_index))) {
        continue; // 跳过未选中的端口
        }

        rtk_uint32 port_bit = 1 << port_index; // 端口位掩码

        // 3.2 配置第一个AFE寄存器 (0xBF84)
        ret_t result = rtl8372n_phy_regbits_write(port_bit, 31, AFE_REG1, REG1_MASK, REG1_VALUE);
        if (result != RT_ERR_OK) return result;

        // 3.3 配置第二个AFE寄存器 (0xBF8C)
        result = rtl8372n_phy_regbits_write(port_bit, 31, AFE_REG2, REG2_MASK, REG2_VALUE);
        if (result != RT_ERR_OK) return result;
    }

    return RT_ERR_OK; // 成功
}

ret_t RL6818C_pwr_on_patch_phy_v008(rtk_uint32 port_mask)
{
    int port_index = 0;                          // 当前处理的端口索引
    ret_t result = RT_ERR_OK;                          // 函数返回值
    const int current_version = 8;               // 目标固件版本
    rtk_uint32 status_value;                    // 状态寄存器值
    rtk_uint32 rtct_status;                    // RTCT状态值
    rtk_uint32 fw_version;                      // 固件版本号

    // 遍历所有端口 (0-7)
    while (port_index < 8) {
        rtk_uint32 port_bit = 1 << port_index;    // 端口位掩码

        // 1.1 检查端口是否在掩码中且状态正常
        if ((port_mask & port_bit) && (uc1_sram_read_8b(port_index, 5) == 2)) {
        // 1.2 检查固件版本
            rtl8372n_phy_write(port_bit, 31, 0xA436, 0x801E);
            rtl8372n_phy_read(port_index, 31, 0xA438, &fw_version);

            // 1.3 如果版本不匹配则执行更新
            if (fw_version != current_version) {
                int retry;

                rtl8372n_phy_regbits_write(port_bit, 31, 0xB820, 0x10, 1);

                // 等待状态就绪 (最多30次重试)
                retry = 30;
                do {
                    rtl8372n_phy_regbits_read(port_index, 31, 0xB800, 0x40, &status_value);
                    if (status_value == 1) break;
                    retry--;
                } while (retry > 0);

                rtl8372n_phy_regbits_write(port_bit, 31, 0xA436, 0xFFFF, 0x8023);
                rtl8372n_phy_regbits_write(port_bit, 31, 0xA438, 0xFFFF, 0x1802);
                rtl8372n_phy_regbits_write(port_bit, 31, 0xA436, 0xFFFF, 0xB82E);
                rtl8372n_phy_regbits_write(port_bit, 31, 0xA438, 0xFFFF, 1);

                rtl8372n_phy_regbits_write(port_bit, 31, 0xB820, 0x80, 1);

                rtl8372n_phy_regbits_write(port_bit, 31, 0xB820, 0x80, 0);

                data_ram_patch_6818C_221026(port_bit);

                rtl8372n_phy_regbits_write(port_bit, 31, 0xA436, 0xFFFF, 0);
                rtl8372n_phy_regbits_write(port_bit, 31, 0xA438, 0xFFFF, 0);
                rtl8372n_phy_regbits_write(port_bit, 31, 0xB82E, 1, 0);

                rtl8372n_phy_regbits_write(port_bit, 31, 0xA436, 0xFFFF, 0x8023);
                rtl8372n_phy_regbits_write(port_bit, 31, 0xA438, 0xFFFF, 0);

                rtl8372n_phy_regbits_write(port_bit, 31, 0xB820, 0x10, 0);

                // 2.11 等待状态空闲
                retry = 30;
                do {
                    rtl8372n_phy_regbits_read(port_index, 31, 0xB800, 0x40, &status_value);
                    if (status_value == 0) break;
                    retry--;
                } while (retry > 0);

                // 启用RTCT功能
                rtl8372n_phy_regbits_write(port_bit, 31, 0xA4A0, 0x400, 1);

                // 等待RTCT就绪
                retry = 30;
                do {
                rtl8372n_phy_regbits_read(port_index, 31, 0xA600, 0xFF, &rtct_status);
                if (rtct_status == 1) break;
                retry--;
                } while (retry > 0);

                // 应用RTCT参数
                RTCT_para_6818C_231206(port_bit);

                // 配置SRAM
                uc1_sram_write_8b(port_index, 0x8FFB, 1);
                uc1_sram_write_8b(port_index, 0x80DC, 0xA);
                uc1_sram_write_8b(port_index, 0x8378, 0x22);

                // 配置PHY寄存器
                rtl8372n_phy_regbits_write(port_bit, 31, 0xA47E, 0xC0, 1);

                // 配置更多SRAM
                uc2_sram_write_8b(port_index, 0x8217, 0x1E);
                uc2_sram_write_8b(port_index, 0x8384, 4);
                uc2_sram_write_8b(port_index, 0x8FD6, 0);
                uc2_sram_write_8b(port_index, 0x8FD7, 0);
                uc2_sram_write_8b(port_index, 0x8FD8, 0xC);
                uc2_sram_write_8b(port_index, 0x8FD9, 0x80);
                uc2_sram_write_8b(port_index, 0x8FDA, 0xA);
                uc2_sram_write_8b(port_index, 0x8FDB, 0x19);
                uc2_sram_write_8b(port_index, 0x8FDC, 0x19);
                uc2_sram_write_8b(port_index, 0x8FDD, 0);
                uc2_sram_write_8b(port_index, 0x8FDE, 0);
                uc2_sram_write_8b(port_index, 0x8FDF, 0);
                uc2_sram_write_8b(port_index, 0x8FE0, 0);
                uc2_sram_write_8b(port_index, 0x8FE1, 0x20);
                uc2_sram_write_8b(port_index, 0x8FE2, 0xC);
                uc2_sram_write_8b(port_index, 0x8FD3, 0);
                uc2_sram_write_8b(port_index, 0x8FD4, 0x15);
                uc2_sram_write_8b(port_index, 0x8FD5, 0x15);

                // 应用AFE补丁
                afe_patch_6818C_220607(port_bit);

                rtl8372n_phy_write(port_bit, 31, 0xA5D0, 0);
                rtl8372n_phy_regbits_write(port_bit, 31, 0xA428, 0x200, 0);
            }
        }
    // 处理下一个端口
    port_index++;
    }
    // 打印完成信息
    rtl_debug("RL6818C_pwr_on_patch_phy_v008 , patch 0x%x finished!\n", port_mask);
    return result;
}


ret_t RL6818C_pwr_on_patch_phy_v008_rls_lockmain(rtk_uint32 port_mask)
{
    ret_t result = port_mask;                          // 函数返回值
    const int current_version = 8;               // 目标固件版本
    // 步骤1: 定义关键寄存器地址
    const rtk_uint32 RTCT_CTRL_REG = 0xA4A0;   // 42144 (RTCT控制寄存器)
    const rtk_uint32 RTCT_STATUS_REG = 0xA600; // 42496 (RTCT状态寄存器)
    const rtk_uint32 FW_CTRL_REG = 0xA436;     // 42038 (固件控制寄存器)
    const rtk_uint32 FW_VERSION_REG = 0xA438;  // 42040 (固件版本寄存器)

    // 步骤2: 定义位掩码和常量
    const rtk_uint32 RTCT_ENABLE_MASK = 0x400; // 位10 (RTCT使能位)
    const rtk_uint32 RTCT_STATUS_MASK = 0xFF;  // 低8位 (RTCT状态位)
    const rtk_uint32 MAX_RETRY = 30;           // 最大重试次数
    const rtk_uint32 FW_CTRL_VALUE = 0x801E;   // 固件控制值

    // 步骤3: 遍历所有端口 (0-7)
    for (int port_index = 0; port_index < 8; port_index++) {
        // 3.1 检查端口是否在掩码中
        if (!(port_mask & (1 << port_index))) continue; // 跳过未选中的端口

        rtk_uint32 port_bit = 1 << port_index; // 端口位掩码

        // 3.2 禁用RTCT功能
        rtl8372n_phy_regbits_write(port_bit, 31, RTCT_CTRL_REG, RTCT_ENABLE_MASK, 0);

        // 3.3 等待RTCT状态空闲
        rtk_uint32 status_value;
        int retry_count = MAX_RETRY;
        do {
            // 读取RTCT状态
            rtl8372n_phy_regbits_read(port_index, 31, RTCT_STATUS_REG, RTCT_STATUS_MASK, &status_value);
            // 检查状态是否为1 (忙状态)
            if (status_value != 1) break;
            retry_count--;
        } while (retry_count > 0);

        // 3.4 恢复固件版本配置
        rtl8372n_phy_regbits_write(port_bit, 31, FW_CTRL_REG, 0xFFFF, FW_CTRL_VALUE);

        // 3.5 设置当前固件版本
        result = rtl8372n_phy_regbits_write(port_bit, 31, FW_VERSION_REG, 0xFFFF, current_version);
    }

    return result;
}

static rtk_api_ret_t _rtk_switch_init_8372n(void)
{
    rtk_uint32 init_state;
    rtk_api_ret_t ret;

    rtl8372n_getAsicRegBits(0x7F60u, 3u, &init_state);
    if (init_state != 2) return RT_ERR_CHIP_NOT_SUPPORTED; //不知道该返回啥了

    // 关键寄存器配置
    rtl8372n_setAsicRegBits(0x6330u, 0x30000u, 0u);   // 0x6330[17:16] 清零
    rtl8372n_setAsicRegBits(0x6330u, 0xC0u, 0u);      // 0x6330[7:6] 清零
    rtl8372n_setAsicRegBits(0x6334u, 0xF0u, 0xFu);    // 0x6334[7:4] 置位 (1111)
    rtl8372n_setAsicRegBits(0x6454u, 0x7000u, 7u);    // 0x6454[14:12] 设置为7 (111)

    msleep(1);

    /* Match the post-reset SDS sequence in Xiaomi's v009 driver. */
    ret = rtl8372n_sds_regbits_write(0, 0, 0, 0x200, 1);
    if (ret != RT_ERR_OK)
        return ret;
    ret = rtl8372n_sds_regbits_write(0, 6, 2, 0x2000, 1);
    if (ret != RT_ERR_OK)
        return ret;
    ret = rtl8372n_sds_regbits_write(1, 7, 16, 0xff, 3);
    if (ret != RT_ERR_OK)
        return ret;

    // rtl8373_setAsicRegBits(0xA90, 0xF, 0xC); //MDI_REVERSE
    // rtl8373_setAsicRegBits(0xA94, 0xFFFF, 0x596A); //TX_POLARITY_SWAP

    msleep(5);
    ret = fw_reset_flow_tgr(1);
    if (ret != RT_ERR_OK)
        return ret;
    msleep(5);
    ret = fw_reset_flow_tgr(0);
    if (ret != RT_ERR_OK)
        return ret;

    // Disable PHYs for configuration
    rtl8372n_phy_write(0xF0u, 0x1fu, 0xA610u, 0x2858u);

    // Set bits 0x13 and 0x14 of 0x5fd4
	// r5fd4:0002914a R5fd4-001a914a
    rtl8372n_setAsicRegBits(0x5FD4u, 0x180000u, 3);

    // 端口寄存器配置 (地址范围: 0x1538-0x1B38, 步进256)
    rtk_uint32 base_addr = 0x1238u;
    for(rtk_uint32 port = 3;port < 9;port++){
        rtl8372n_setAsicRegBits(base_addr + (port * 0x100u), 0x10u, 1);// 设置当前地址的位4 (0x10)
        rtl8372n_setAsicRegBits(base_addr + (port * 0x100u), 0x100u, 1);// 设置当前地址的位8 (0x100)
        // rtl8372n_setAsicRegBits(base_addr + (port * 0x100u), 0x40, 0);
    }

    // r0b7c:000000d8 R0b7c-000000f8 r6040:00000030 R6040-00000031
    rtl8372n_setAsicRegBits(0x0B7Cu, 0x20u, 1);

    // 初始化寄存器块 (地址范围: 0x7124-0x714C, 步进4)
    rtk_uint32 reg_index = 0x7124u;
    do
    {
        rtl8372n_setAsicReg(reg_index, 0x1050u);// 写入固定值 0x1050
        reg_index += 4;                           // 步进4
    }
    while ( reg_index != 0x714C );               // 结束地址: 29004 (0x714C)

    //Clock register ?
    rtl8372n_setAsicRegBits(0x6040u, 1u, 1);
    msleep(100);

    RL6818C_pwr_on_patch_phy_v008(0xF0u);     // 应用电源管理补丁 (所有端口)
    RL6818C_pwr_on_patch_phy_v008_rls_lockmain(240LL);// 应用锁相环稳定性补丁 240ms

    // Re-enable PHY after configuration
    rtl8372n_phy_write(0xF0u, 0x1fu, 0xA610u, 0x2058u);

	// Enables MAC access
	// Set bits 0xc-0x14 of 0x632c to 0x1f8, see rtl8372_init
	// r632c:00000540 R632c-001f8540 // RTL8373: 001ff540
    rtl8372n_setAsicRegBits(0x632Cu, 0x1FF000u, 0x1F8u);
    msleep(50);
    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_switch_initialState_set
 * Description:
 *      Set initial status
 * Input:
 *      state   - Initial state;
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK       - Initialized
 *      RT_ERR_FAILED   - Uninitialized
 * Note:
 *
 */
rtk_api_ret_t rtk_switch_initialState_set(init_state_t state)
{
    if(state >= INIT_STATE_END)
        return RT_ERR_FAILED;

    init_state = state;
    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_switch_probe
 * Description:
 *      Probe switch
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK       - Switch probed
 *      RT_ERR_FAILED   - Switch Unprobed.
 * Note:
 *
 */
rtk_api_ret_t rtk_switch_probe(switch_chip_t *pSwitchChip)
{

    rtk_uint32 retVal = RT_ERR_FAILED;
    rtk_uint32 data, regValue;
    if((rtl8372n_getAsicReg(4, &regValue)) != RT_ERR_OK)
        return retVal;

    data = regValue >> 8;
    switch (data)
    {
        case 0x837300u:
            *pSwitchChip = CHIP_RTL8373;
            break;
        case 0x837200u:
            *pSwitchChip = CHIP_RTL8372;
            break;
        case 0x837370u:
            *pSwitchChip = CHIP_RTL8373N;
            break;
        case 0x837270u:
            *pSwitchChip = CHIP_RTL8372N;
            break;
        default:
            return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_switch_init
 * Description:
 *      Set chip to default configuration environment
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 *      RT_ERR_SMI          - SMI access error
 * Note:
 *      The API can set chip registers to default configuration for different release chip model.
 */
static rtk_api_ret_t rtk_switch_init_chip(switch_chip_t switchChip)
{
    rtk_uint32  retVal;

    switch(switchChip)
    {
        case CHIP_RTL8372N:
            if((retVal = _rtk_switch_init_8372n()) != RT_ERR_OK)
                return retVal;
            break;
        case CHIP_RTL8372:
        case CHIP_RTL8373:
        case CHIP_RTL8373N:
            return RT_ERR_CHIP_NOT_SUPPORTED;
        default:
            return RT_ERR_CHIP_NOT_FOUND;
    }

    /* Set initial state */

    if((retVal = rtk_switch_initialState_set(INIT_COMPLETED)) != RT_ERR_OK)
        return retVal;

    rtl_debug("Rtl837x Finish Init switch\n");
    return RT_ERR_OK;
}

rtk_api_ret_t rtk_switch_init(void)
{
    rtk_uint32 retVal;
    switch_chip_t switchChip;

    rtl_debug("Rtl837x Start Init switch\n");
    if((retVal = rtk_switch_probe(&switchChip)) != RT_ERR_OK)
        return retVal;

    return rtk_switch_init_chip(switchChip);
}

rtk_api_ret_t rtk_switch_init_by_chip(switch_chip_t switchChip)
{
    rtl_debug("Rtl837x Start Init switch\n");
    return rtk_switch_init_chip(switchChip);
}
