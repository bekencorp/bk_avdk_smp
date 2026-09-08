# BK7258 Secureboot XIP OTA 测试矩阵

> 产品：Direct-XIP A/B（boot_param）  
> 默认 `try_max` / sticky LIMIT = **5**  
> 用例源：`projects/secureboot_xip/tests/ota_xip/cases.json`  
> 工具：`ota_xip_test.py` + `boot_param_tool.py`

## 关键不变量（判据）

| 场景 | 行为 |
|------|------|
| NORMAL | `try==5` sticky 一次到另一可用槽；`try>5` 清 try、不再 sticky |
| TRIAL | `try≤try_max` 跑 update；`try>try_max` 回滚 exec 并清 try |
| OTA AES | 仅 `flash_aes_type=FIXED`；XIP 包始终软件 CRC |
| try 计数 | AON_PMU（暖复位累加；冷启动清零）；非 flash `try_count` |
| boot_param 烧录地址 (BK7258) | **0x7fc000**（8K） |

## 建议执行顺序

### Phase1-baseline

| ID | 说明 |
|----|------|
| P-01 | NONE: OTA 仅 CRC 不 AES |
| H-01 | A→B 首次升级并 confirm |
| T-01 | TRIAL 成功 confirm（默认 try_max=5） |
| E-01 | sig_verify_en=FALSE：只校哈希 |

### Phase2-recovery

| ID | 说明 |
|----|------|
| C-01 | 坏 header 不 CBUS 卡死 |
| S-03 | preferred 不可用 → 直接选另一可用槽 |
| T-02 | TRIAL 新镜像 boot 挂死 → 回滚 |
| T-03 | TRIAL 边界：try==try_max 仍试新槽 |
| T-04 | TRIAL 边界：try>try_max 回滚 |
| S-01 | preferred header OK / body hang → sticky 一次 |
| S-02 | sticky 目标也挂 → try>5 清计数不二次 sticky |

### Phase3-power-sec

| ID | 说明 |
|----|------|
| R-01 | OTA 写入中断电 |
| R-02 | set_trial 后、首次 boot 前断电 |
| R-03 | TRIAL 已进 AP、confirm 前复位 |
| R-04 | confirm 写 boot_param 中断电 |
| E-02 | sig_verify_en=TRUE：错签名拒 |
| E-03 | 篡改 image 哈希失败 |

## 工具用法（摘要）

```bash
cd projects/secureboot_xip/tests/ota_xip
python3 ota_xip_test.py plan --priority P0
python3 ota_xip_test.py prepare H-01
python3 ota_xip_test.py prepare p0 --build-dir ../../../../build/bk7258/secureboot_xip/bk7258/_build
python3 ota_xip_test.py check-pack --build-dir <_build> --aes none --record
python3 ota_xip_test.py record H-01 pass --note '...'
python3 ota_xip_test.py report --markdown
```

`prepare` 已覆盖的 ID 在下表「Prepare」列为 Yes。

## 统计

- 总用例：**45**
- 优先级：P0=21，P1=20，P2=4
- 分类：Happy=5, Negative=6, Pack=5, Power=6, Security=6, SoftCRC=4, Sticky=5, Trial=8

## 完整用例表

### Pack

| ID | Pri | Prepare | 场景 | 前置 | 步骤要点 | 期望 |
|----|-----|---------|------|------|----------|------|
| P-01 | P0 | Yes | NONE: OTA 仅 CRC 不 AES | security flash_aes_type=NONE；ota.csv 无 encrypt | 完整 pack，检查中间产物与 ota.bin | 生成 ota_crc.bin；无 ota_aes.bin；ota.bin 含 OTA hdr + CRC 布局 |
| P-02 | P0 | Yes | FIXED: OTA AES+CRC | flash_aes_type=FIXED + 合法 64 hex key | 完整 pack | ota_aes.bin → ota_aes_crc.bin → ota.bin；CONFIG_OTA_ENCRYPTED=1 |
| P-03 | P1 | Yes | encrypt 字段可缺省 | ota.csv 不含 encrypt；security=NONE | gen _ota.h + pack | 不报错；CONFIG_OTA_ENCRYPTED=0；行为同 P-01 |
| P-04 | P1 | — | 版本号写入 OTA 镜像 | ota.csv app_version=1.2.3；security_counter=N | pack 后解析 ota_signed / 烧录后 header | ih_ver / security_counter 与 csv 一致 |
| P-05 | P1 | — | all-app 与 ota 一致性 | 同一次 build 的 all-app.bin 与 ota.bin | 对比 primary payload（去 OTA hdr / 对齐 CRC） | payload 与对应 slot 镜像一致（含签/哈希） |

### Happy

| ID | Pri | Prepare | 场景 | 前置 | 步骤要点 | 期望 |
|----|-----|---------|------|------|----------|------|
| H-01 | P0 | Yes | A→B 首次升级并 confirm | 当前 NORMAL exec=A；合法更高版本 ota 写入 B | OTA 写 slotB → set_trial(B) → reboot → AP 起来 → confirm | TRIAL 选 B；confirm 后 NORMAL exec=B；try 清 0；冷启仍 B |
| H-02 | P0 | Yes | B→A 反向升级并 confirm | NORMAL exec=B | 同 H-01，目标 A | 对称于 H-01；最终 NORMAL exec=A |
| H-03 | P0 | — | 连续两次 OTA（乒乓） | 完成 H-01 后立刻再升一版 | A/B 交替升级两次，每次都 confirm | 每次均落在新 slot；无 residual TRIAL；try 始终被清 |
| H-04 | P1 | — | 同版本/低版本升级策略 | 目标镜像 version ≤ 当前 | 发起 OTA / 或仅烧录低版本到非活跃槽后 reboot | 按产品策略：拒绝下载或 boot 仍选更高/BP preferred（记录实测） |
| H-05 | P1 | Yes | 仅刷新非活跃槽后不 set_trial | NORMAL；写坏/写新到非活跃槽但不 arm TRIAL | reboot | 仍 boot exec_slot；不进入 TRIAL 回滚路径 |

### Trial

| ID | Pri | Prepare | 场景 | 前置 | 步骤要点 | 期望 |
|----|-----|---------|------|------|----------|------|
| T-01 | P0 | Yes | TRIAL 成功 confirm（默认 try_max=5） | 合法新镜像；set_trial | reboot 1 次内 AP 调 confirm | state→NORMAL；exec=running；PMU try clear |
| T-02 | P0 | Yes | TRIAL 新镜像 boot 挂死 → 回滚 | update 槽 header/签名可通过，但 app 进不去或死循环复位；try_max=5 | 连续 warm reset，累计 try 到 >5（第 6 次） | decide: rollback → NORMAL exec=旧槽；try clear；能进旧系统 |
| T-03 | P0 | Yes | TRIAL 边界：try==try_max 仍试新槽 | try_max=5 | 第 5 次 boot（try_cnt=5）观察 decide 日志 | 仍 preferred=update_slot；尚未 rollback |
| T-04 | P0 | Yes | TRIAL 边界：try>try_max 回滚 | 承接 T-03 | 第 6 次 boot | rollback commit；boot 旧 exec_slot |
| T-05 | P1 | Yes | TRIAL 期间不触发 NORMAL sticky | TRIAL；人为让 try 到 5 | 看 loader 日志有无 sticky/pre-swap | in_trial 时不做 sticky；回滚只由 decide_slot 负责 |
| T-06 | P1 | — | confirm 时非 TRIAL 幂等 | 已是 NORMAL | 再调 ota_boot_param_confirm | skip / return 0；不改坏 record |
| T-07 | P1 | — | confirm 采用实际 running slot | TRIAL；若 sticky/异常曾改过实际启动槽 | confirm 后读 boot_param | exec_slot == bk_ota_get_current_partition() |
| T-08 | P2 | Yes | 自定义 try_max | record.try_max=3（合法 <15） | 坏镜像 TRIAL 不 confirm | try_cnt>3 时回滚；日志 try_max=3 |

### Sticky

| ID | Pri | Prepare | 场景 | 前置 | 步骤要点 | 期望 |
|----|-----|---------|------|------|----------|------|
| S-01 | P0 | Yes | preferred header OK / body hang → sticky 一次 | NORMAL；BP preferred=坏槽（header 软 CRC 过、验签/跑挂）；另一槽好图 | 连续 warm reset 至 try==5 | try==5 sticky 到另一可用槽并成功 boot；有 verified exceed 日志 |
| S-02 | P0 | Yes | sticky 目标也挂 → try>5 清计数不二次 sticky | 两槽都 body hang（或 sticky 目标不可用） | try 到 >5 | try cleared, no sticky；candidate 保持 version/BP；不乒乓死循环 |
| S-03 | P0 | Yes | preferred 不可用 → 直接选另一可用槽 | BP preferred header 坏（soft CRC/hook 失败）；另一槽好 | reboot | slot_available[pref]=false；boot 好槽；不依赖 sticky |
| S-04 | P1 | — | 成功 boot 后 try 清零 | NORMAL 正常镜像 | 多次冷/热启成功进系统 | 稳定后 PMU try=0（或每次成功路径 clear）；无误 sticky |
| S-05 | P1 | — | 不可 sticky 到 unavailable | try==5；对端槽不可用 | 观察 swap 逻辑 | 不切换到 unavailable；保持 candidate |

### SoftCRC

| ID | Pri | Prepare | 场景 | 前置 | 步骤要点 | 期望 |
|----|-----|---------|------|------|----------|------|
| C-01 | P0 | Yes | 坏 header 不 CBUS 卡死 | 目标槽前若干字节破坏 CRC 布局 | BL2 扫双槽 header | DBUS soft CRC 失败 → BOOT_EBADIMAGE；设备不 hang；另一槽可 boot |
| C-02 | P0 | Yes | 全 0xFF 空槽 | 非活跃槽擦除未写 | boot | 该槽 unavailable；boot 活跃好槽 |
| C-03 | P1 | — | plaintext header hook 二次校验 | 软 CRC 过但 header magic/结构非法 | boot | hook 判坏；不进 CBUS 盲读路径挂死 |
| C-04 | P1 | — | 双槽都坏 header | A/B header 均破坏；非 TRIAL | boot | 无可用槽；行为符合产品（panic/复位）；try 按 NORMAL 策略清理 |

### Security

| ID | Pri | Prepare | 场景 | 前置 | 步骤要点 | 期望 |
|----|-----|---------|------|------|----------|------|
| E-01 | P0 | Yes | sig_verify_en=FALSE：只校哈希 | 当前工程配置；合法 hash 镜像 | 正常 OTA + boot | 验签跳过；哈希失败仍拒 boot |
| E-02 | P0 | — | sig_verify_en=TRUE：错签名拒 | 打开验签；故意错 privkey 签 ota | 烧录/OTA 后 boot | 该槽 unavailable / 验签失败；回退另一槽或拒绝 |
| E-03 | P0 | Yes | 篡改 image 哈希失败 | 合法 ota 写入后改 body 1 byte | reboot | 校验失败不进系统；不 hang |
| E-04 | P1 | — | FIXED AES 通路端到端 | flash_aes_type=FIXED；芯片 AES key 匹配 | pack→烧 all-app→OTA 升一版→confirm | CBUS/DBUS 路径正确；升级成功 |
| E-05 | P1 | — | NONE 与 FIXED 镜像不可混用 | 设备 NONE；刷 FIXED 打包的 ota（或反之） | OTA / boot | 校验或读出失败；不砖（另一槽可救） |
| E-06 | P2 | — | security_counter 单调 | 新镜像 counter < 已接受 | OTA/boot（若启用 anti-rollback） | 按 MCUboot/产品策略拒绝 |

### Power

| ID | Pri | Prepare | 场景 | 前置 | 步骤要点 | 期望 |
|----|-----|---------|------|------|----------|------|
| R-01 | P0 | — | OTA 写入中断电 | 下载写非活跃槽中途断电 | 上电；必要时重试 OTA | 仍可从旧槽 boot；坏半包被 softCRC/校验丢弃 |
| R-02 | P0 | — | set_trial 后、首次 boot 前断电 | 已写好镜像并 commit TRIAL | 断电再上电 | 进入 TRIAL 路径；成功则 confirm，失败可回滚 |
| R-03 | P0 | — | TRIAL 已进 AP、confirm 前复位 | 新固件起来但未 confirm | 多次复位不调用 confirm | try 累加；超限回滚旧槽（T-02） |
| R-04 | P1 | — | confirm 写 boot_param 中断电 | TRIAL 成功路径；写 ping-pong 时掉电 | 反复上电 | ab_flag 双扇区可恢复到一致合法记录；最终能 NORMAL 或再试 TRIAL |
| R-05 | P1 | — | 冷启动清 PMU try | 人为 try 抬到非 0 | 真正断电 PON（非 soft reset） | try=0；不误 sticky |
| R-06 | P1 | — | warm reset 保留 try | TRIAL 或 NORMAL hang 场景 | WDT/软件复位 | try 递增保留，支撑回滚/sticky |

### Negative

| ID | Pri | Prepare | 场景 | 前置 | 步骤要点 | 期望 |
|----|-----|---------|------|------|----------|------|
| N-01 | P0 | Yes | OTA 包截断 / 长度错 | 截短 ota.bin | HTTP/本地 OTA | 下载层或校验失败；不 set_trial；旧系统可用 |
| N-02 | P1 | — | 错误 OTA global/img hdr | 篡改 OTA 头 checksum/offset | OTA | 拒绝写入或写后不可 boot 新槽 |
| N-03 | P1 | Yes | boot_param 全毁 | 擦除 boot_param 分区 | reboot | virgin → slot A；可再 OTA 重建 record |
| N-04 | P1 | Yes | boot_param CRC 坏单扇区 | 破坏 ping 或 pong 一侧 | reboot / confirm | 读到另一侧合法 latest；可继续 |
| N-05 | P2 | — | 非法 preferred slot 值 | 手工写坏 exec/update slot 字段 | reboot | decide force A；不越界 |
| N-06 | P2 | — | 并发二次 OTA | 第一次 TRIAL 未结束又发起下载 | 产品 CLI/HTTP 重入 | 拒绝或串行；不出现双 TRIAL 撕裂 |

## 日志锚点

- BL2：`bp override` / `decide: TRIAL|NORMAL|rollback` / `try>5 cleared, no sticky` / `verified exceed` / SoftCRC|`BOOT_EBADIMAGE`
- AP：`boot_param trial armed` / `confirm done` / `secure xip: armed trial`

## H-01 快速步骤（示例）

1. 确认当前 NORMAL 在 A（可选烧 `prepare H-01` 的 boot_param @ 0x7fc000）
2. `ota.csv` 抬高 `app_version`（如 0.0.2）后重新 pack
3. HTTP 提供 `ota.bin`；板上 `http_ota http://<PC>:8080/ota.bin`
4. 见 `armed trial for slot 1` 后复位 → TRIAL 进 B → AP 自动 confirm
5. 冷启仍在 B → `record H-01 pass`

