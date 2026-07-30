## 简介

- 支持通过 DHCP 为客户端分配 IP，无需手动固定 IP。
- 自动过滤来自上级路由的 DHCP 报文（OFFER / ACK 等），防止干扰 U-Boot DHCP（IPQ50xx 暂不支持）。
- 类 Argon 风格的 Web 界面，针对移动端也进行了优化。

> [!NOTE]
>
> - 京东云哪吒/后羿无法启动 6.18 内核的固件 ([pmyy-wt/jdc_re-cs-03](https://github.com/pmyy-wt/jdc_re-cs-03/releases) 仓库 2026-07-27 及之后的固件)。
> - IPQ50xx 机型暂不支持 [uBootEnter](https://github.com/chenxin527/uBootEnter)。
