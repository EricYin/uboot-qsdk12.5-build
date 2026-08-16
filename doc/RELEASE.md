## 新增设备

| 平台    | 设备                                   | 型号              | 备注                                            |
| :------ | :------------------------------------- | :---------------- | :---------------------------------------------- |
| IPQ53xx | Xiaomi BE3600 Pro (5/8 Ethernet ports) | xiaomi_be3600-pro | 该机型正式版可能有锁，若有锁则不要刷写此 U-Boot        |
| IPQ60xx | AnySafe E1                             | anysafe_e1        | 待测试                                           |
| IPQ60xx | Redmi AX5                              | redmi_ax5         | 待测试                                           |
| IPQ60xx | Xiaomi AX1800                          | xiaomi_ax1800     | 待测试                                           |
| IPQ807x | Cradlepoint E320                       | cradlepoint_e320  | 原机 CPU 有锁，需更换无锁 CPU 才能刷写此 U-Boot      |
| IPQ807x | OPPO CKB01 (SoftBank Air 5G)           | oppo_ckb01        |                                                 |
| IPQ807x | Xiaomi AX9000                          | xiaomi_ax9000     | 待测试                                           |

## 新增闪存驱动

| 类型     | 型号                     | 状态   | 备注                                                         |
| :------- | :----------------------- | :----- | :----------------------------------------------------------- |
| SPI-NOR  | W25Q512NW                | 待测试 | 参考: [Commit 7d846f2](https://github.com/1980490718/u-boot-2016/commit/7d846f2f8b275b03f7bfc5a792dc19f8538fd688) |
| SPI-NAND | F50D4G41XB               | 待测试 | 参考: [Commit 0ecd64d](https://github.com/1980490718/u-boot-2016/commit/0ecd64dd0614768c8102f54c478f680c7a11fc15) |
| SPI-NAND | GD5F4GQ4R/GD5F4GQ6R 系列 | 待测试 | 参考: [Commit 507d75c](https://github.com/1980490718/u-boot-2016/commit/507d75c3fd61aad081b671ba255051b3bef1c032) |

## 新特性

- U-Boot 启动时打印设备信息（设备型号和 config_name）。
- 支持刷写纯 NOR 固件（分区表参考高通 [meta-tools](https://github.com/chenxin527/meta-tools) 中的 nor-partition.xml），暂不支持刷写单 firmware 分区的纯 NOR 固件。

## BUG 修复

- 修复网络命令 (ping, tftpboot, tftpput 等) 在网页终端/Telnet 终端下执行后 httpd 可能失联的问题。
- 修复网页终端下部分命令执行时间过长导致 TCP 连接超时的问题。
- 修复 JDCloud BE6500 的 factory 固件解析失败的问题（将 factory 固件 kernel 大小限制调整为 1 MiB 的整数倍）。
- 修复 CMIOT AX18、Qihoo 360V6、Redmi AX5 JDCloud 和 ZN M2 部分网口不通的问题。

## 优化

- 只在 BOOTCONFIG 分区数据有效时执行 bootconfig 命令，避免用户主动擦除了 BOOTCONFIG 分区的情况下执行该命令导致固件刷写结果返回失败。
- 优化 9008 模式下的 MIBIB 自动重载逻辑。
- 优化 tftp/wget 文件传输进度、传输速率及文件大小信息显示。
- IPQ53xx/IPQ95xx: 防止长时间无网络活动导致 PPE 硬件休眠。
- wget/flashread: 当用户未指定加载地址且 loadaddr 环境变量未设置时，根据设备内存大小自动设置默认加载地址。

## 其他

- 默认开启 httpd_debug 模式，打印详细的日志信息，便于调试。
- 调整 CMIOT AX18、Redmi AX5 JDCloud 和 ZN M2 的 LED 配置。
- 精简部分机型包含的额外 DTB，减小 U-Boot 大小。

> [!NOTE]
>
> - 京东云哪吒/后羿无法启动 6.18 内核的固件 ([pmyy-wt/jdc_re-cs-03](https://github.com/pmyy-wt/jdc_re-cs-03/releases) 仓库 2026-07-27 及之后的固件)。
> - IPQ50xx 机型暂不支持 [uBootEnter](https://github.com/chenxin527/uBootEnter)。
