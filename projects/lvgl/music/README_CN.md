# LVGL Music 工程中文说明

本工程基于 LVGL 官方音乐播放器演示，结合音频播放器组件bk_audio_player，实现的本地音乐播放器。

## 1. 目录结构
```
music/
├── CMakeLists.txt                 # 顶层 CMake 构建入口
├── Makefile                       # Make 构建入口
├── pj_config.mk                   # 项目配置开关
├── ap/                            # AP 核业务代码
│   ├── ap_main.c                  # LVGL 初始化、LCD 打开、触摸配置
│   ├── lv_demo_music*.c/.h        # LVGL 音乐 UI、播放控制、列表管理
│   ├── cli_lv_demo_music.*        # lv_music CLI 命令及压力测试逻辑
│   ├── assets/                    # 频谱贴图、Logo 等 UI 资源
│   └── config/                    # BK7258 平台 AP 侧配置
├── cp/                            # CP 核辅助代码，负责上电 CP1
│   ├── cp_main.c
│   └── config/
├── partitions/                    # 分区与 RAM 区域配置
│   └── bk7258/
└── .ci/.gitignore/.it.csv         # CI 与忽略文件
```

## 2. 功能特点

### 2.1 图形界面
- 采用 `lv_demo_music_main.c` 中的自定义布局，包含专辑封面、频谱动画、播放进度条、循环/随机按钮以及可滑动歌单列表。
- 支持 480x854 ST7701S RGB LCD（在 `ap_main.c` 中配置 `lcd_device_st7701s` 与背光 GPIO），并可通过 `CONFIG_TP` 选项启用触摸屏。
- 使用 `lv_vendor` 框架提供的多帧缓存与局部刷新，保证动画流畅度。

### 2.2 音频播放链路
- 若 `CONFIG_AUDIO_PLAYER` 使能，`lv_demo_music.c` 会挂载 `/sd0` FATFS，递归扫描 SD 卡（默认路径 `/sd0`），采集歌曲元数据并将文件加入 `bk_audio_player` 播放队列。
- 通过 `bk_audio_metadata_get_from_file` 自动读取标题、歌手、流派、时长，若缺失则退回到文件名解析（`lv_demo_music_list_mgr.c`）。
- 支持的音频格式由 Kconfig 决定（MP3/WAV/AAC/M4A/FLAC/OGG/OPUS/AMR/TS 等），仅在启用对应解码器后文件才会被加入列表。
- UI 与 audio_player 通过事件回调同步进度、切歌、暂停/恢复及拖动 Seek。
- 未插入 SD 卡或未检测到音频文件时，界面仍可正常演示 UI，播放功能自动降级为占位信息。

## 3. 支持的操作

### 3.1 触控界面
- **播放/暂停**：点击中间播放按钮触发 `_lv_demo_music_play`/`_lv_demo_music_pause`，同步驱动 `bk_audio_player`.
- **上一曲/下一曲**：左右箭头触发 `_lv_demo_music_album_next`，除顺序循环外，还支持随机模式。
- **进度拖动**：滑动进度条触发 `_lv_demo_music_request_seek`，LVGL 完成动画后由音频线程执行真正的 `bk_audio_player_seek`。
- **循环模式**：循环按钮固定为顺序循环；随机按钮可切换 `AUDIO_PLAYER_MODE_RANDOM`，随机播放不会重复最近曲目。
- **歌单选择**：在列表区域滑动选择任意歌曲条目并点击即可播放；若启用了自动播放（`LV_DEMO_MUSIC_AUTO_PLAY`），界面将自动切换展示。

## 4. 环境准备
- **硬件**：BK7258 开发板 + ST7701S RGB LCD + 触摸板（可选）+ SD 卡模块 + 扬声器/功放。
- **存储**：SD 卡需为 FAT/FAT32，挂载路径 `/sd0`；将支持格式的音频文件放入根目录或子目录，工程会递归扫描。
- **软件配置**：
  - 在 menuconfig 中勾选 `CONFIG_LVGL`, `CONFIG_LV_USE_DEMO_MUSIC`, `CONFIG_AUDIO_PLAYER` 及所需解码器。
  - 若使用触摸，确认 `CONFIG_TP` 及设备驱动已开启。
  - 根据 LCD/触摸硬件如需调整 GPIO，可修改 `ap_main.c` 内 `bk_display_rgb_ctlr_config_t` 与背光控制函数。

## 5. 编译与烧录
```
make bk7258 PROJECT=lvgl/music
```
生成的固件烧录至开发板后，上电即可自动启动 LVGL 界面。确保 AP 串口波特率正确，方便查看系统日志与交互提示。

## 6. 运行流程
1. 断电插入已准备好的 SD 卡，连接 LCD、触摸及音频输出。
2. 上电后观察串口日志，确认 `Mounted SD card to /sd0` 和 `Audio player initialized successfully` 等信息。
3. 待 LVGL 启动画面结束后，可直接触摸 UI 进行播放控制。
4. 测试完成后可拔出 SD 卡或在串口执行 `reboot` 重新启动演示。

## 7. 常见问题
- **无声音/歌单为空**：检查 SD 卡文件系统、音频格式是否在 Kconfig 里启用；串口若提示 `No music files found` 说明扫描失败，可使用 `ls /sd0` 验证挂载。
- **触摸无响应**：确认 `CONFIG_TP` 已开启且硬件 I2C 接线正确，必要时在 `drv_tp_open` 处调整分辨率或镜像参数。
- **播放跳转卡顿**：若频繁快进/快退导致卡顿，可适当降低操作频率或放宽每次拖动的间隔时间，避免音频线程过载。
