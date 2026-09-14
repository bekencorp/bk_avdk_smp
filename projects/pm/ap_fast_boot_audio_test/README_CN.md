# AP Fast Boot Audio 测试工程

* [English](./README.md)

## 1. 工程概述

本工程验证 BK7259 AP+CP 上，音频服务在 **AP Fast Boot（cold）** 下的停服 / 下电 / 恢复。产品侧走 PM 回调，不依赖 CLI；本工程用 CLI 模拟产品行为。

覆盖三个服务：

- **voice**：通话 / 环回（mic + AEC + speaker）
- **asr**：本地 KWS（本工程已开 `CONFIG_BEKEN_KWS` / NPU）
- **player**：独立播音（playback）或混进 voice 喇叭的提示音（prompt_tone）

Cold 路径：`stop` 拆对象、保留 cfg；fast boot 后自动再 `init`。`deinit` 拆对象并清 cfg，醒来不再恢复。Hot（寄存器备份）不在本工程范围。

## 2. 适用硬件与环境

- 推荐开发板：BK7259
- 一块板即可（喇叭 + 麦克风）
- 两路串口：CP 控制台、AP 控制台（日志与 CLI）
- 本工程 defconfig 已开：`CONFIG_PM_AP_FAST_BOOT_ENABLE`、`CONFIG_AUD_PM_FAST_COLD`

## 3. 目录结构

```
pm/ap_fast_boot_audio_test/
├── ap/
│   ├── audio_test/          # aud_fb 统一入口（推荐）
│   ├── voice_service_test/
│   ├── asr_service_test/
│   ├── player_service_test/
│   └── config/bk7259_ap/
├── cp/
│   └── aud_fb_ipc/          # AP 请求 CP 投 AP OFF
├── common/
├── Makefile
└── CMakeLists.txt
```

## 4. 编译与烧录

在 SDK 根目录：

```
make bk7259 PROJECT=pm/ap_fast_boot_audio_test
```

固件：`build/bk7259/ap_fast_boot_audio_test/package/all-app.bin`

烧录后分别打开 **CP 串口** 和 **AP 串口**。

## 5. CLI 约定

### 5.1 从哪条串口敲

| 串口 | 写法 |
|------|------|
| CP | `ap_cmd aud_fb ...`（转发到 AP） |
| AP | `aud_fb ...` |

下文一律写 `ap_cmd` 形式；在 AP 串口去掉前面的 `ap_cmd` 即可。

底层 CLI（`voice_service` / `asr_service` / `player_service`）也可直接用，但 **请走 `aud_fb`**：它会记 running 位、区分 `stop`/`deinit`，最后一个服务停下后向 CP 申请 AP OFF。

### 5.2 stop 和 deinit

| 命令 | 对象 | cfg / want_restart | fast boot 后 |
|------|------|--------------------|--------------|
| `stop` | 拆掉 | **保留** | 自动再 `init`（**主验证路径**） |
| `deinit` | 拆掉 | **清除** | **不再恢复** |

产品空闲一般是 `stop`（还要醒来继续干活），真正关掉才 `deinit`。

### 5.3 自动 AP OFF 与唤醒

默认 `auto_ap_off=on`：最后一个服务 `stop`/`deinit` 后，AP 向 CP 申请 AP OFF。

```
# 看当前 running 位
ap_cmd aud_fb service status

# 关掉自动下电（调试时自己控制）
ap_cmd aud_fb auto_ap_off off

# 强制申请 AP OFF（调试）
ap_cmd aud_fb ap off
```

AP 下电后，在 **CP 串口** 唤醒：

```
ap_fast_boot on
```

成功标志：AP 再次打 `RESUME_PROOF`，且刚 `stop` 过的服务会按 cfg 拉起来。`main_entries` 仍为 1（fast boot，不是冷复位重跑 `main`）。

### 5.4 命令一览

```
ap_cmd aud_fb service voice init [参数]
ap_cmd aud_fb service voice stop
ap_cmd aud_fb service voice deinit
ap_cmd aud_fb service asr init [参数]
ap_cmd aud_fb service asr stop
ap_cmd aud_fb service asr deinit
ap_cmd aud_fb service player init [参数]
ap_cmd aud_fb service player stop
ap_cmd aud_fb service player deinit
ap_cmd aud_fb service status
ap_cmd aud_fb auto_ap_off on|off
ap_cmd aud_fb ap off
```

无参数时的默认：

| 服务 | `init` 默认 | 含义 |
|------|-------------|------|
| voice | `onboard 8000 AEC g711a / onboard 8000 单声道` | 环回 |
| asr | `startwithmic onboard 16000` | ASR **自占 mic** |
| player | `playback array 0` | player **自占喇叭** |

## 6. 单独测某个 service

每条用例：起来 → 确认声音/识别 → `stop` → CP `ap_fast_boot on` → 确认自动恢复。

### 6.1 只测 voice（环回）

```
ap_cmd aud_fb service voice init
# 对麦说话，喇叭应有环回；AP 日志 ONBOARD_MIC / RAW_READ / ONBOARD_SPK 有流量
ap_cmd aud_fb service voice stop
ap_fast_boot on
# 应再次听到环回，无需再 init
```

自定义参数（与 `voice_service start` 相同）：

```
ap_cmd aud_fb service voice init onboard 8000 1 g711a g711a onboard 8000 1
```

恢复后不想再起 voice：

```
ap_cmd aud_fb service voice deinit
ap_fast_boot on
# voice 不应再起来
```

### 6.2 只测 ASR（自带 mic）

```
ap_cmd aud_fb service asr init
# 先说唤醒词「你好博通」，再测下面命令词
ap_cmd aud_fb service asr stop
ap_fast_boot on
```

本工程 KWS：

| 说法 | 日志 |
|------|------|
| 你好博通 | `nihaobotong, cmd: 1` |
| 再见博通 | `zaijianbotong, cmd: 2` |
| Play Music | `play music, cmd: 3` |
| Stop Play | `stop play, cmd: 4` |
| Next song | `next song, cmd: 5` |
| Volume Up | `volume up, cmd: 6` |
| Volume Down | `volume down, cmd: 7` |

显式自带 mic：

```
ap_cmd aud_fb service asr init startwithmic onboard 16000
```

**不要**再执行 `voice init`：两路 ADC 会冲突（dump）。

### 6.3 只测 player（独立喇叭）

```
ap_cmd aud_fb service player init
# 听到内置 array 提示音后
ap_cmd aud_fb service player stop
ap_fast_boot on
```

等价于 `playback array 0`。这条路径 **自己占喇叭**，不要和 voice 同时开。

## 7. Service 组合

### 7.1 能组 / 不能组

| 组合 | 做法 | 说明 |
|------|------|------|
| voice + player 提示音 | 先 voice，再 `prompt_tone` | 产品多路混音标准测法 |
| asr 跟 voice 共用 mic | **只** `asr init startnomic` | nomic 会自己拉起 voice，不要再 `voice init` |
| voice + asr startwithmic | **禁止** | 双 ADC |
| voice + player playback | **不要一起开** | 两套喇叭，环回会被打掉 |

### 7.2 voice + player 提示音（推荐组合）

提示音 PCM 混进 voice 的 speaker，player 自己不占 DAC。

```
ap_cmd aud_fb service voice init
ap_cmd aud_fb service player init prompt_tone 0 array 0
# 应听到提示音，同时环回还在
```

`prompt_tone` 参数：`prompt_tone <tone_id> array <array_id>`

| array_id | 内容 |
|----------|------|
| 0 | PCM 唤醒提示（16 kHz） |
| 1 | MP3 配网提示 |
| 2 | WAV 低电提示 |

**先停 player，再停 voice**（产品也是这个顺序）：

```
ap_cmd aud_fb service player stop
ap_cmd aud_fb service voice stop
ap_fast_boot on
```

恢复后：voice 先起来，再挂 mix 并重播提示音。AP 日志应有 `mix attached` / `prompt_tone resume started`，然后听到提示音。

一次性提示音已经播完、醒来 **不要再播**：用 `player deinit` 而不是 `stop`。

反序（先 voice 再 player）现在也能停干净，但喇叭已拆，只会 skip mix detach。

### 7.3 ASR + voice（nomic，共用一条 voice）

```
ap_cmd aud_fb service asr init startnomic onboard 16000
# 不要再 voice init
# 先「你好博通」，再测命令词
ap_cmd aud_fb service asr stop
ap_fast_boot on
```

`startnomic` 内部 `bk_voice_init`，ASR 读这条 voice 的 mic。再 `voice init` 会双开。

当前 nomic **不能**挂到已经在跑的 voice 上；要跟已起的 voice 组合，需要改代码。

### 7.4 三个一起（voice 环回 + 提示音 + 独立 ASR）

独立 ASR 占自己的 mic，和 voice 冲突，**不要** `startwithmic` + voice。

可行的是：nomic ASR（自带 voice）+ 提示音：

```
ap_cmd aud_fb service asr init startnomic onboard 16000
ap_cmd aud_fb service player init prompt_tone 0 array 0
ap_cmd aud_fb service player stop
ap_cmd aud_fb service asr stop
ap_fast_boot on
```

或：手动 voice + 提示音（无 ASR），见 7.2。

## 8. 建议自测清单

1. voice only：`init` → 环回 → `stop` → `ap_fast_boot on` → 环回恢复
2. voice only：`init` → `deinit` → `ap_fast_boot on` → **不**恢复
3. asr only：`init` → 唤醒词 → `stop` → fast boot → 仍能识别
4. player playback only：`init` → 出声 → `stop` → fast boot
5. voice + prompt_tone：7.2 全流程，恢复后提示音再响
6. asr startnomic：7.3，不要额外 `voice init`

## 9. 常见日志与诊断

| 现象 | 查看 |
|------|------|
| 命令成功 | `CMDRSP:OK` |
| 环回在跑 | `count_ut`：`ONBOARD_MIC` / `RAW_READ` / `ONBOARD_SPK` |
| 提示音在播 | `onboard_speaker_stream_set_input_port_info` 带非空 port，随后 `PLAYER_EVENT_FINISH` |
| 申请下电 | `no audio service running -> request CP AP OFF` |
| Fast boot 成功 | `RESUME_PROOF` 继续涨，`main_entries=1` |
| 提示音恢复失败 | 应有 `wait mix attach` → `mix attached` → `prompt_tone resume started`；立刻 `FINISH` 且没有 mix 则挂接失败 |
| ASR 识别 | `nihaobotong, cmd: 1` 等 |

不要用 `aud_fb ap on` 把 AP 拉起来，AP 不能给自己投票上电，用 CP 的 `ap_fast_boot on`。

## 10. 关键文件

- `ap/audio_test/cli_aud_fb_test.c`：统一 CLI、stop/deinit、自动 AP OFF
- `ap/voice_service_test/cli_voice_service.c`
- `ap/asr_service_test/cli_asr_service.c`
- `ap/player_service_test/cli_player_service.c`：prompt_tone 混音与 resume 挂接
- `cp/aud_fb_ipc/`：AP → CP 申请 AP OFF
- 服务实现：`sdk/ap/components/bk_voice_service`、`bk_asr_service`、`bk_player_service`（PM API 均在 `CONFIG_AUD_PM_FAST_COLD` 下）
