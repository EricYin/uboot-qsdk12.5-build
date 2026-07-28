## 简介

- 支持通过 DHCP 为客户端分配 IP，无需手动固定 IP。
- 自动过滤来自上级路由的 DHCP 报文（OFFER / ACK 等），防止干扰 U-Boot DHCP（IPQ50xx 暂不支持）。
- 类 Argon 风格的 Web 界面，针对移动端也进行了优化。
