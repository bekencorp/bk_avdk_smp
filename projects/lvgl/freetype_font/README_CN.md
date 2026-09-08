# LVGL FreeType Font 工程中文说明

本工程演示 LVGL FreeType 字体加载能力，通过 LittleFS 从 flash 中读取 `Lato-Regular.ttf`，并在 BK7258 平台上创建自定义字体文本。

## 1. 目录结构
```
freetype_font/
├── CMakeLists.txt                 # 顶层 CMake 构建入口
├── Makefile                       # Make 构建入口
├── ap/                            # AP 核业务代码
│   ├── ap_main.c                  # LVGL、QSPI LCD、VFS 和 FreeType 初始化
│   ├── resource/                  # LittleFS 字体资源及烧录说明
│   └── config/                    # BK7258 AP 侧配置
├── cp/                            # CP 核配置
│   └── config/
└── partitions/                    # 分区与 RAM 区域配置
```

## 2. 功能特点
- 挂载 `BK_PARTITION_USR_CONFIG` 对应的 LittleFS 分区，路径为 `VFS_INTERNAL_FLASH_PATITION_0`。
- 从 LittleFS 读取 `Lato-Regular.ttf`，并通过 `lv_ft_font_init()` 创建 24 号 FreeType 字体。
- 在屏幕中心显示使用 FreeType 字体渲染的文本。
- 默认使用 `lcd_device_st77903_h0165y008t` QSPI LCD。
- 可通过 `CONFIG_TP` 启用触摸输入。

## 3. 资源准备
`ap/resource/readme.txt` 中说明：需要使用烧录工具将 `littlefs.bin` 烧录到 usr 分区，默认地址为 `0x3eb000`。如果分区表调整过，需要同步改为当前 `BK_PARTITION_USR_CONFIG` 的起始地址。

LittleFS 中应包含：
```
Lato-Regular.ttf
```

## 4. 硬件与配置
- 硬件：BK7258 开发板、`lcd_device_st77903_h0165y008t` 对应 QSPI LCD、触摸屏（可选）。
- LCD 控制：QSPI ID 为 0，复位脚默认 GPIO40，背光 GPIO7。
- 软件配置：确认 `CONFIG_LVGL`、LVGL FreeType 支持、`CONFIG_VFS` 和 LittleFS 相关配置已开启。

## 5. 编译与烧录
```
make bk7258 PROJECT=lvgl/freetype_font
```

烧录应用固件后，还需要将 `littlefs.bin` 烧录到 usr 分区；否则运行时会提示字体文件打开失败。

## 6. 运行流程
1. 准备包含 `Lato-Regular.ttf` 的 LittleFS 镜像。
2. 烧录工程固件和 LittleFS 镜像。
3. 复位开发板，观察串口日志中 LittleFS mount、文件读取和 `lv_ft_font_init()` 结果。
4. 屏幕中心出现 `Hello world` 文本后，说明 FreeType 字体加载成功。

## 7. 常见问题
- **file_content open failed**：LittleFS 镜像未烧录、分区地址错误，或镜像中缺少 `Lato-Regular.ttf`。
- **create failed**：确认 LVGL FreeType 配置已开启，字体文件完整且内存足够。
- **屏幕无显示**：检查 QSPI LCD 型号、复位脚 GPIO40、背光 GPIO7 和 PSRAM 供电状态。
