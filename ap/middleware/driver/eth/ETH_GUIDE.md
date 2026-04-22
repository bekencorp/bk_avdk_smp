# BK7259 Ethernet 使用指南

## 1. 概述

BK7259 内置 10/100/1000M Ethernet MAC，支持 **RMII** 和 **GRMII** 两种 PHY 接口模式，通过 `CONFIG_ETH_GPHY` 配置项切换。驱动基于 LwIP 协议栈，支持 DHCP 和静态 IP 两种寻址方式。

当前支持的 PHY 芯片：

| CONFIG 选项 | PHY 型号 | 速率 |
|---|---|---|
| `CONFIG_PHY_SMSC=y` | SMSC LAN8742 系列 | 10/100M |
| `CONFIG_PHY_MICREL_KSZ90X1=y` | Micrel KSZ90x1 系列 | 10/100/1000M |
| `CONFIG_PHY_REALTEK=y` | Realtek 系列 | - |

## 2. 引脚连接

当前代码使用 GPIO 40-55 引脚组 (FUNC_CODE_130)。RMII 模式和 GRMII 模式共享基础引脚，GRMII 模式额外使用 TXD2/TXD3/RXD2/RXD3/GTCLK/GRCLK。

### 2.1 RMII 模式引脚 (默认, CONFIG_ETH_GPHY 未定义)

| RMII 信号 | 方向 | GPIO | PHY 侧引脚 | 说明 |
|---|---|---|---|---|
| PHY_INT | PHY→MCU | GPIO_55 | nINT | PHY 中断 |
| MDC | MCU→PHY | GPIO_47 | MDC | MDIO 管理时钟 |
| MDIO | 双向 | GPIO_48 | MDIO | MDIO 管理数据 |
| RXD0 | PHY→MCU | GPIO_51 | RXD0 | 接收数据位 0 |
| RXD1 | PHY→MCU | GPIO_52 | RXD1 | 接收数据位 1 |
| RXDV | PHY→MCU | GPIO_50 | CRS_DV | 载波侦测 / 接收数据有效 |
| TXD0 | MCU→PHY | GPIO_44 | TXD0 | 发送数据位 0 |
| TXD1 | MCU→PHY | GPIO_43 | TXD1 | 发送数据位 1 |
| TX_EN | MCU→PHY | GPIO_45 | TX_EN | 发送使能 |
| REF_CLK | 取决于配置 | GPIO_49 | REFCLK | 50MHz RMII 参考时钟 |

### 2.2 GRMII 模式引脚 (CONFIG_ETH_GPHY=y)

在 RMII 基础引脚之上，额外使用以下引脚：

| GRMII 信号 | 方向 | GPIO | PHY 侧引脚 | 说明 |
|---|---|---|---|---|
| RXD2 | PHY→MCU | GPIO_53 | RXD2 | 接收数据位 2 |
| RXD3 | PHY→MCU | GPIO_54 | RXD3 | 接收数据位 3 |
| TXD2 | MCU→PHY | GPIO_42 | TXD2 | 发送数据位 2 |
| TXD3 | MCU→PHY | GPIO_41 | TXD3 | 发送数据位 3 |
| GRCLK | PHY→MCU | GPIO_49 | RX_CLK | 125MHz 接收时钟 (替代 REF_CLK) |
| GTCLK | MCU→PHY | GPIO_40 | GTX_CLK | 125MHz 发送时钟 |

> **注意**: GRMII 模式下 GPIO_49 映射为 `ENET_GRCLK` (125MHz RX 时钟)，不再是 RMII 的 `ENET_REF_CLK`。

### 2.3 REF_CLK 与 GTCLK / GRCLK 的区别

| 时钟信号 | 频率 | 所属接口 | 说明 |
|---|---|---|---|
| **REF_CLK** | 50 MHz | RMII | RMII 模式唯一时钟，收发共用 |
| **GTCLK** | 125 MHz | GRMII / RGMII | 千兆模式 TX 发送时钟 |
| **GRCLK** | 125 MHz | GRMII / RGMII | 千兆模式 RX 接收时钟 |

### 2.4 典型连接示例

**RMII 模式 (100M PHY, 如 LAN8742):**

```
BK7259 MCU                          LAN8742 PHY
===========                         ===========
GPIO_47 (MDC)     ──────────────>   MDC
GPIO_48 (MDIO)    <────────────>    MDIO
GPIO_45 (TX_EN)   ──────────────>   TXEN
GPIO_44 (TXD0)    ──────────────>   TXD0
GPIO_43 (TXD1)    ──────────────>   TXD1
GPIO_50 (RXDV)    <──────────────   CRS_DV
GPIO_51 (RXD0)    <──────────────   RXD0
GPIO_52 (RXD1)    <──────────────   RXD1
GPIO_49 (REF_CLK) <──────────────   nINT/REFCLK (50MHz)
GPIO_55 (PHY_INT) <──────────────   (可选中断脚)
```

**GRMII 模式 (1000M PHY):**

```
BK7259 MCU                          1000M PHY
===========                         =========
GPIO_47 (MDC)     ──────────────>   MDC
GPIO_48 (MDIO)    <────────────>    MDIO
GPIO_45 (TX_EN)   ──────────────>   TX_EN
GPIO_44 (TXD0)    ──────────────>   TXD0
GPIO_43 (TXD1)    ──────────────>   TXD1
GPIO_42 (TXD2)    ──────────────>   TXD2
GPIO_41 (TXD3)    ──────────────>   TXD3
GPIO_40 (GTCLK)   ──────────────>   GTX_CLK (125MHz)
GPIO_50 (RXDV)    <──────────────   RX_DV
GPIO_51 (RXD0)    <──────────────   RXD0
GPIO_52 (RXD1)    <──────────────   RXD1
GPIO_53 (RXD2)    <──────────────   RXD2
GPIO_54 (RXD3)    <──────────────   RXD3
GPIO_49 (GRCLK)   <──────────────   RX_CLK (125MHz)
GPIO_55 (PHY_INT) <──────────────   (可选中断脚)
```

### 2.5 PHY 接口选择寄存器

ETH MAC Reg0x804 的 `phy_intf_sel` 字段 [17:14] 控制 PHY 接口类型，由驱动在初始化时根据 `CONFIG_ETH_GPHY` 自动设置：

| phy_intf_sel 值 | 接口模式 | CONFIG 条件 |
|---|---|---|
| 0x1 | GRMII | `CONFIG_ETH_GPHY=y` |
| 0x4 | RMII | `CONFIG_ETH_GPHY` 未定义 |

## 3. CONFIG 配置项

在 `projects/<your_project>/config/<board>/defconfig` 中配置：

### 3.1 基础配置 (必选)

```
CONFIG_ETH=y                  # 启用 Ethernet 功能
CONFIG_PHY_SMSC=y             # 选择 PHY 芯片驱动 (根据实际硬件)
```

### 3.2 PHY 接口模式

```
CONFIG_ETH_GPHY=y             # 启用 GRMII 千兆 PHY 支持 (默认 n, 即 RMII)
```

### 3.3 网络地址配置

```
CONFIG_ETH_DHCP=y             # 启用 DHCP 自动获取 IP (默认 y)
                              # 设为 n 则使用静态 IP
```

### 3.4 可选功能

| CONFIG 选项 | 默认值 | 说明 |
|---|---|---|
| `CONFIG_ETH_GPHY` | n | GRMII 千兆 PHY 支持 |
| `CONFIG_ETH_CSUM_OFFLOAD` | y | 硬件校验和卸载 (IP/TCP/UDP/ICMP) |
| `CONFIG_ETH_PTP` | n | IEEE 1588-2008 PTP 精确时间协议 |
| `CONFIG_ETH_VLAN` | n | VLAN 支持 |
| `CONFIG_ETH_TSO` | n | TCP 分段卸载 |
| `CONFIG_ETH_PM_CB_SUPPORT` | n | 低功耗回调支持 (进入/退出低压模式) |
| `CONFIG_ETH_LPI` | n | 低功耗空闲 (当前 BK7259 硬件不支持) |
| `CONFIG_ETH_EEE` | n | Wake-on-LAN |
| `CONFIG_ETH_REGISTER_CALLBACKS` | n | 使用注册回调模式 |

### 3.5 最小 defconfig 示例

**RMII 100M (默认):**

```
CONFIG_ETH=y
CONFIG_PHY_SMSC=y
```

**GRMII 1000M:**

```
CONFIG_ETH=y
CONFIG_ETH_GPHY=y
CONFIG_PHY_MICREL_KSZ90X1=y
```

## 4. 初始化流程

系统启动后 Ethernet 自动初始化，流程如下:

```
ethernetif_init()
  └── low_level_init()
        ├── HAL_ETH_Init()                 # MAC 硬件初始化
        │     └── HAL_ETH_MspInit()        # 底层硬件配置
        │           ├── GPIO PinMux 配置    # 根据 CONFIG_ETH_GPHY 配置引脚复用
        │           ├── AHBP 电源域上电
        │           ├── ETH 时钟使能        # sys_hal_set_eth_clk_en(1)
        │           ├── ETH 软复位
        │           ├── PHY 接口选择        # eth_set_phy_intf_sel() RMII/GRMII
        │           ├── 注册中断 ISR
        │           └── 使能 ETH 中断
        ├── PHY 连接 & 自协商              # phy_connect() + phy_startup()
        └── 启动 MAC 收发                  # HAL_ETH_Start_IT()
```

应用层无需手动调用初始化函数，LwIP 协议栈会自动完成网络接口注册和 DHCP/静态 IP 配置。

## 5. 注意事项

1. **PHY 电源**: 确保 PHY 芯片上电时序正确，PHY 复位引脚由硬件控制。
2. **REF_CLK 方向**: RMII 的 50MHz 参考时钟可由 PHY 提供 (常见) 或 MCU 提供，需根据 PHY 数据手册和硬件设计确定。
3. **PHY 驱动选择**: 同一时刻只能启用一个 PHY 驱动 CONFIG，根据板载 PHY 型号选择。
4. **GRMII 额外引脚**: 启用 `CONFIG_ETH_GPHY` 后会额外占用 GPIO_40/41/42/53/54，确认这些引脚未被其他外设使用。
5. **时钟域**: RMII 使用 50MHz REF_CLK，GRMII 使用 125MHz GTCLK/GRCLK，两种模式的时钟信号不可混用。
