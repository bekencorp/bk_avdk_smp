# LVGL Image Decode 工程中文说明

本工程演示 LVGL 图片解码与显示能力，通过 LittleFS 从 flash 中读取图片资源，并在 BK7258 平台上使用 LVGL 显示解码后的图片。

## 1. 目录结构
```
img_decode/
├── CMakeLists.txt                 # 顶层 CMake 构建入口
├── Makefile                       # Make 构建入口
├── ap/                            # AP 核业务代码
│   ├── ap_main.c                  # LVGL、LCD、VFS 和图片解码初始化
│   ├── resource/                  # LittleFS 图片资源及烧录说明
│   └── config/                    # BK7258 AP 侧配置
├── cp/                            # CP 核配置
│   └── config/
└── partitions/                    # 分区与 RAM 区域配置
```

## 2. 功能特点
- 挂载 `BK_PARTITION_USR_CONFIG` 对应的 LittleFS 分区，路径为 `VFS_INTERNAL_FLASH_PATITION_0`。
- 默认调用 `lv_jpeg_img_load_with_hw_dec()` 从 LittleFS 加载 `/img/anim/anim-0.jpg` 并使用硬件 JPEG 解码。
- 代码中保留 JPEG 软件解码和 PNG 加载示例，可按需切换；PNG 需要开启 `LV_USE_PNG`。
- 支持 LVGL v8 和非 v8 两套图片对象接口。
- 默认适配 ST7701S RGB LCD，并通过 `lv_vendor` 局部刷新显示图片。

## 3. 资源准备
`ap/resource/readme.txt` 中说明：需要使用烧录工具将 `littlefs.bin` 烧录到 usr 分区，默认地址为 `0x3eb000`。如果分区表调整过，需要同步改为当前 usr 分区地址。

默认运行路径需要 LittleFS 中包含：
```
/img/anim/anim-0.jpg
```

如切换为代码中注释的示例路径，需要在镜像中放入对应图片文件。

## 4. 硬件与配置
- 硬件：BK7258 开发板、ST7701S RGB LCD、触摸屏（可选）。
- LCD 控制：默认 GPIO0/GPIO12/GPIO1/GPIO6，背光 GPIO7，LCD LDO GPIO13。
- 软件配置：确认 `CONFIG_LVGL`、`CONFIG_VFS`、LittleFS 和 JPEG 解码相关配置已开启；如需 PNG，开启 `LV_USE_PNG`。

## 5. 编译与烧录
```
make bk7258 PROJECT=lvgl/img_decode
```

烧录应用固件后，还需要将包含图片资源的 `littlefs.bin` 烧录到 usr 分区；否则图片加载会失败。

## 6. 运行流程
1. 准备包含 `/img/anim/anim-0.jpg` 的 LittleFS 镜像。
2. 烧录工程固件和 LittleFS 镜像。
3. 复位开发板，观察串口日志中 LittleFS mount 和图片解码结果。
4. 图片居中显示在屏幕上后，说明资源读取和解码显示链路正常。

## 7. 常见问题
- **图片不显示**：确认 LittleFS 镜像已烧录到 usr 分区，且图片路径与代码中的路径一致。
- **PNG 加载失败**：确认 `LV_USE_PNG` 已开启，并使用 `lv_png_img_load()` 对应路径。
- **JPEG 解码失败**：检查图片格式是否支持硬件解码；必要时切换为软件解码接口验证。
