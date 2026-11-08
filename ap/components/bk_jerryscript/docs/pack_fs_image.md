# 通用 littlefs 应用镜像打包说明

本文说明如何为任意工程制作 `bk_jerryscript` 可挂载的 littlefs 应用镜像（bin）。
不依赖具体工程，`launcher_v9` 的实际用法见 `../README.md`。

## 1. 前置条件

| 项目 | 要求 |
| --- | --- |
| 打包工具 | `ap/components/littlefs/mkimg/mklittlefs`（Linux 可执行文件） |
| 组件配置 | `CONFIG_LITTLEFS=y`、`CONFIG_VFS=y`、`CONFIG_BK_JERRYSCRIPT_FS=y` |
| 自动挂载 | 需要上电即可用时再打开 `CONFIG_BK_JERRYSCRIPT_FS_AUTO_MOUNT` |
| 挂载点 | `CONFIG_BK_JERRYSCRIPT_APPS_ROOT`，默认 `/apps` |
| Flash 分区 | 一个足够大的 data 分区，推荐直接复用 `usr_config` |

## 2. 三个关键参数

打包参数必须与运行期一致，否则镜像能生成但挂载失败或数据错乱。

### 2.1 block / page

由 littlefs 的块设备适配层决定。`ap/components/bk_vfs/littlefs_adapter.c` 中：

```c
config->read_size  = 256;
config->prog_size  = 256;
config->block_size = 4096;
```

所以打包时固定：

```text
-b 4096  -p 256
```

> QSPI NAND 方案使用 128KB block / 2048B page，需要按同一份适配层配置调整。

### 2.2 size

必须**恰好等于分区大小**，因为 `block_count = 分区大小 / block_size`。
大小从编译产物读取：

```bash
build/<chip>/<project>/partitions/partitions.txt
```

例如：

```text
usr_config                      0x004c9000         1024K
```

则 `-s 1048576`（1MB）。分区地址用于烧录，分区大小用于 `-s`。

### 2.3 挂载点与目录层级

镜像根目录 = 挂载点。默认挂载到 `/apps`，因此镜像根目录下**直接**放应用目录：

```text
/                      <- 镜像根，挂载后就是 /apps
├── app_a/
│   ├── manifest.json
│   ├── main.js
│   └── lib/helper.js
└── app_b/
    ├── manifest.json
    └── main.js
```

不要再套一层 `apps/`，否则运行期路径会变成 `/apps/apps/app_a/main.js`。

## 3. 通用打包流程

```bash
# SDK 根目录
SDK=/path/to/bk_avdk_smp

# 打包工具
MKFS=$SDK/ap/components/littlefs/mkimg/mklittlefs

# 待打包目录（镜像根）
APP_ROOT=/tmp/fs_root

# 分区大小（字节），取自 partitions.txt
FS_SIZE=1048576

# 输出镜像
IMAGE=littlefs_app.bin
```

### 3.1 准备应用目录

```bash
mkdir -p "$APP_ROOT/app_a"

# 单文件应用
cp main.js "$APP_ROOT/app_a/main.js"

# 或者多个 JS 按依赖顺序合并（公共库在前）
cat ui_common.js app_a.js main.js > "$APP_ROOT/app_a/main.js"
```

### 3.2 写 manifest

```bash
cat > "$APP_ROOT/app_a/manifest.json" <<'EOF'
{
  "id": "app_a",
  "name": "App A",
  "version": "1.0.0",
  "entry": "main.js"
}
EOF
```

约定：

- `id` 与目录名保持一致，`bk_js_app_start("<id>")` 按它定位。
- `entry` 为相对应用目录的入口文件名。
- `name`、`version` 仅用于展示和排查，可省略。

### 3.3 打包

```bash
"$MKFS" -c "$APP_ROOT" -b 4096 -p 256 -s "$FS_SIZE" "$IMAGE"
```

产物大小固定为 `FS_SIZE`，不足部分自动补齐，可直接整片烧录。

## 4. 模块与公共库

应用内可用 `require()` 加载同目录下的其他脚本：

```text
app_a/
├── manifest.json
├── main.js
└── lib/
    ├── draw.js
    └── http.js
```

```js
var draw = require('lib/draw.js');
```

运行期 `require()` 的源码副本分配在 PSRAM，不会挤占 SRAM 堆。

## 5. 烧录与验证

```bash
# 分区起始地址来自 partitions.txt，例如 usr_config = 0x004c9000
# 方式一：SDK 的烧录目标（按所用版本参数为准）
make <chip> PROJECT=<project> flash

# 方式二：通用烧录工具，例如 esptool
esptool.py --port /dev/ttyUSB0 write_flash 0x004c9000 littlefs_app.bin
```

核心只有一点：镜像整片写到分区的**起始地址**，不用改分区表，也不用重新编译固件。

正常启动日志：

```text
ap0:js:I(xxx):mounted /apps on usr_config (0x4c9000, 1048576 bytes)
ap0:js:D(xxx):  app_a
ap0:js:D(xxx):read /apps/app_a/main.js: <字节数>
ap0:js:I(xxx):js app 'app_a' running from fs
```

## 6. 常见问题

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `mounted` 后无任何目录 | 打包目录层级多套了一层 | 镜像根直接放应用目录 |
| `cannot open /apps/<id>/main.js` | `manifest.json` 缺失或 `entry` 写错 | 检查清单和文件名大小写 |
| `littlefs mount ... failed` | `-s` 与分区大小不一致、地址不对 | 对齐 `partitions.txt` |
| 读到乱码 / 解析失败 | `-b/-p` 与适配层不一致 | 统一用 4096 / 256（NOR） |
| 挂载后目录存在但文件 0 字节 | 打包源文件为空或路径写错 | 检查 `ls -l "$APP_ROOT"` |
| `no memory for ...` | 脚本缓冲分配失败 | 确认 PSRAM 可用，源码缓冲走 PSRAM |

## 7. 注意事项

- 换工程只需改 `APP_ROOT`、`FS_SIZE`、`IMAGE` 三个变量，命令本身通用。
- 修改 JS 后重新打包并烧录即可，不需要重新编译固件。
- 只替换文件内容而不改大小时，也可以在原镜像上做增量更新，但要求预先格式化镜像；
  简单起见推荐每次整片重打包、整片烧录。
- 当前版本的 `mklittlefs` 在 `-l` / `-u` 读取路径上可能挂起，校验请以板端启动日志为准。
