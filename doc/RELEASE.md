## 新增设备

| 平台    | 设备                                   | 型号              | 备注                                            |
| :------ | :------------------------------------- | :---------------- | :---------------------------------------------- |
| IPQ53xx | Xiaomi BE3600 Pro (5/8 Ethernet ports) | xiaomi_be3600-pro | 该机型正式版可能有锁，若有锁则不要刷写此 U-Boot |
| IPQ807x | OPPO CKB01 (SoftBank Air 5G)           | oppo_ckb01        |                                                 |

## 新特性

- U-Boot 启动时打印设备信息（设备型号和 config_name）。
- 支持刷写纯 NOR 固件（分区表参考高通 [meta-tools](https://github.com/chenxin527/meta-tools) 中的 nor-partition.xml）。
- 新增对 W25Q512NW (SPI-NOR) 的支持。

## 其他

- 只在 BOOTCONFIG 数据有效时执行 bootconfig 命令，避免用户主动擦除了 0:BOOTCONFIG 分区的情况下执行该命令导致固件刷写结果返回失败。
- flashread: 当用户未指定加载地址且 loadaddr 环境变量未设置时，使用 CONFIG_SYS_LOAD_ADDR 作为默认加载地址。
- 默认开启 httpd_debug 模式，打印详细的日志信息，便于调试。
- 优化 MIBIB 重载逻辑。

> [!NOTE]
>
> - 京东云哪吒/后羿无法启动 6.18 内核的固件 ([pmyy-wt/jdc_re-cs-03](https://github.com/pmyy-wt/jdc_re-cs-03/releases) 仓库 2026-07-27 及之后的固件)。
> - IPQ50xx 机型暂不支持 [uBootEnter](https://github.com/chenxin527/uBootEnter)。
