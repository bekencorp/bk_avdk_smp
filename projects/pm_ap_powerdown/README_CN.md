# AP 低压掉电示例项目

* [English](./README.md)

## 功能概述
- 演示 CP 进入低压睡眠（low voltage）时对 AP（CPU1/CPU2）掉电
- 默认启动时不拉起 AP，便于验证 AP 掉电后的低压功耗
- 支持 RTC、GPIO 等唤醒源将系统从低压唤醒
- 可通过 CLI 手动上电/关闭 AP

* 有关低功耗应用开发，请参阅：

  - `低功耗开发指南 <../../../developer-guide/power_save/index.html>`_

* 有关 Power Manager CLI 说明，请参阅：

  - `Power Manager 示例 <../../../examples/platform/bk_pwr.html>`_

## 主要组件
- `bk_pm`: 电源管理组件
- `pwr_clk`: 电源/时钟控制（含 AP 上电与掉电）
- `cli_pwr`: 低功耗 CLI（`pm` / `pm_vote` / `pm_boot_cp1` / `pm_debug`）

## CLI命令使用
本工程默认不启动 AP，命令在 **CP 串口** 执行。

```bash
pm_boot_cp1 9 0     # 拉起 AP（module=9:APP，0:上电）
pm_boot_cp1 9 1     # 关闭 AP（1:掉电）
pm 1 1 0 0 12 10000 0 1                 # 进入低压，RTC 10s 后唤醒
pm 1 0 8 9 12 9 2 1000000000            # 进入低压，GPIO9 上升沿唤醒
pm_debug 8          # 打印当前电源/低压状态
```

参数说明：

- `pm_boot_cp1 [module_name] [ctrl_state]`
  - `module_name`: `9` 表示 `PM_BOOT_CP1_MODULE_NAME_APP`
  - `ctrl_state`: `0` 上电，`1` 掉电
- `pm [sleep_mode] [wake_source] [vote1] [vote2] [vote3] [param1] [param2] [param3]`
  - `sleep_mode`: `0` normal sleep，`1` low voltage，`2` deep sleep
  - `wake_source`: `0` GPIO，`1` RTC，`2` WIFI/BT，`4` Touch
  - `vote1/vote2/vote3`: `8` BT，`9` WIFI，`12` APP
  - GPIO：`param1` 为 GPIO ID，`param2` 为触发方式（`0` 低电平，`1` 高电平，`2` 上升沿，`3` 下降沿）
  - RTC：`param1` 为唤醒时间，单位 ms

## 日志输出
- AP 上电/掉电状态
- 进入低压、唤醒源回调
- 错误信息

## 测试示例

### 测试环境
- 开发板：Armino 开发板（BK7258）
- 串口：CP 串口用于输入命令；AP 串口用于确认 AP 是否启动
- 可选：精密电源，用于测量低压电流和 VDDDIG 电压

### pm_ap_powerdown 固件编译
- 编译命令：

```bash
make bk7258 PROJECT=pm_ap_powerdown
```

编译完成后烧录 AP/CP 镜像，复位开发板。默认情况下 CP 不会投票拉起 AP，AP 串口无启动日志。

### 测试CASE 1  - AP 上电测试
- CASE命令:

```bash
pm_boot_cp1 9 0
```

- CASE预期结果：

```
成功拉起 AP
```
- CASE成功标准：

```
AP 串口出现启动日志，AP 开始运行
```
- CASE成功日志：

```
boot_cp1 9 0 0x0 [0][0x...]E_1
boot_cp1 9 0 0x200 [0]E_2
```
- CASE失败标准：

```
AP 串口无启动日志，AP 未运行
```
- CASE失败日志：

```
cp0 boot cp1[...] time out, boot cp1 fail!!!
```

### 测试CASE 2  - AP 掉电测试
- CASE命令:

```bash
pm_boot_cp1 9 1
```

- CASE预期结果：

```
AP 掉电，CP 继续运行
```
- CASE成功标准：

```
AP 串口停止输出，CP 侧打印 Shutdown_cp1
```
- CASE成功日志：

```
Shutdown_cp1[0][0][0]
```
- CASE失败标准：

```
AP 仍在运行，未掉电
```

- CASE失败日志：

```
无 Shutdown_cp1 日志
```

### 测试CASE 3  - RTC 唤醒低压测试（AP 掉电）
- CASE命令:

```bash
pm 1 1 0 0 12 10000 0 1
```

- CASE预期结果：

```
系统进入低压，约 10s 后 RTC 唤醒；低压期间 AP 保持掉电
```
- CASE成功标准：

```
低压期间 VDDDIG 降至设定电压（例如 0.6V）；到达设定时间后系统被唤醒；AP 串口无输出
```
- CASE成功日志：

```
cli_pm_cmd 1 1 0 0 12 10000 0!!!
cli_pm_rtc_callback[1]
```
- CASE失败标准：

```
未进入低压，或到达设定时间后系统未被唤醒
```
- CASE失败日志：

```
lowvol1 0x1 0xFFFFEFFF 0xFFFFFFFF
```

说明：`lowvol1` 中第二个值为当前已投票，第三个值为进入低压所需票值。两者不相等表示仍有模块未投票，无法进入低压。可通过 `pm_debug 8` 进一步确认。

### 测试CASE 4  - GPIO 唤醒低压测试（AP 掉电）
- CASE命令:

```bash
pm 1 0 8 9 12 9 2 1000000000
```

- CASE预期结果：

```
系统进入低压；GPIO9 上升沿触发后唤醒；低压期间 AP 保持掉电
```
- CASE成功标准：

```
低压期间 VDDDIG 降至设定电压（例如 0.6V）；GPIO9 从低拉高后系统被唤醒
```
- CASE成功日志：

```
cli_pm_cmd 1 0 8 9 12 9 2!!!
cli_pm_gpio_callback[0]
```
- CASE失败标准：

```
未进入低压，或 GPIO 触发后系统未被唤醒
```
- CASE失败日志：

```
无 cli_pm_gpio_callback 日志
```

GPIO 接线说明：
- 上升沿唤醒：测试前 GPIO 接低电平，唤醒时接高电平
- 下降沿唤醒：测试前 GPIO 接高电平，唤醒时接低电平

## 注意事项
- 默认 SMP 工程在低压时 AP 保持上电（WFI）。本工程开启 `CONFIG_PM_AP_POWERDOWN_WHEN_LV`，低压时会对 AP 掉电，功耗更低。
- `cp/cp_main.c` 中已注释 `bk_pm_module_vote_boot_cp1_ctrl()`，上电后 AP 默认不启动。需要 AP 时先执行 `pm_boot_cp1 9 0`。
- 低功耗相关命令在 CP 侧执行。AP 启动后，发往 AP 的命令需加 `ap_cmd` 前缀。
- 追求更低功耗时，进入低压前关闭蓝牙：`AT+BLEPOWER=0`。
- 进入低压前应结束上层业务。若进低压失败，可在 CP 串口执行 `pm_debug 8` 查看未投票模块。
- 本工程与 `ap_powerdown_keepalive` 不同：本工程演示 AP 掉电后的低压睡眠；后者额外演示 AP 掉电后 CP keepalive 保活与远端唤醒。