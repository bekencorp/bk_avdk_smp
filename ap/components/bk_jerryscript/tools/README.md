# js2c.py — JavaScript 转 C 字符串工具

把 `.js` 源码转成 C 字符串常量，随固件一起编译，运行时交给 `bk_js_eval()` 执行。
适用于板子没有文件系统分区、无法运行时读文件的场景。

脚本不依赖任何工程结构，也不依赖 JerryScript 的构建脚本，可被任意工程直接调用。

- 脚本：`ap/components/bk_jerryscript/tools/js2c.py`
- 依赖：Python 3，无第三方包

## 1. 快速开始

```bash
# 单个脚本 -> 头文件（内部是 static 数组，include 即用）
python3 js2c.py ui.js -o ui_js.h

# 多个脚本按给定顺序合并 -> .c + .h
python3 js2c.py ui_common.js launcher.js main.js \
    -o js_bundle.c --header js_bundle.h --name js_bundle

# 整个目录（--recursive 含子目录，文件按名称排序）
python3 js2c.py ap/js -o js_bundle.c --header js_bundle.h \
    --name js_bundle --recursive
```

生成后把 `.c` 加入工程编译源列表即可，无需其它改动。

## 2. 两种产物形态

### 2.1 输出 `.c`（默认，推荐）

生成一个字符串数组和两个访问函数，同时可按需生成声明头文件。

```c
/* 生成在 js_bundle.c 中 */
const char s_js_bundle[] = "..." ;

const char *js_bundle_source(void);   /* 以 NUL 结尾的 JS 源码 */
unsigned int js_bundle_len(void);     /* 字节数，不含结尾 NUL */
```

对应的 `js_bundle.h` 已经声明好这两个函数，直接 `#include` 使用。

使用示例：

```c
#include "bk_javascript.h"
#include "js_bundle.h"

void ui_start(void)
{
    bk_js_init(NULL);
    bk_js_eval("js_bundle", js_bundle_source(), js_bundle_len(), 0);
}
```

### 2.2 输出 `.h`（单文件内嵌）

不生成 `.c`，数组以 `static const char <var>[]` 的形式直接写在头文件里，适合很小、
只用在一个编译单元的脚本。

```bash
python3 js2c.py ui.js -o ui_js.h
```

```c
#include "ui_js.h"    /* 之后可直接使用 s_ui_js / sizeof(s_ui_js) - 1 */
```

注意：`static` 数组会在每个包含该头文件的编译单元里各存一份，脚本较大时请改用 `.c` 形态。

## 3. 参数说明

| 参数 | 说明 |
| --- | --- |
| `inputs...` | 输入 `.js` 文件或目录，**顺序即合并顺序**（目录内按文件名排序） |
| `-o, --output` | 输出文件，必须以 `.c` 或 `.h` 结尾（必填） |
| `--header` | 额外生成的声明头文件，仅 `.c` 输出可用 |
| `--name` | 符号前缀，默认取输出文件名。生成 `<name>_source()` / `<name>_len()` |
| `--var` | 数组名，默认 `s_<输出文件名>` |
| `--array-only` | 只生成数组，不生成访问函数 |
| `--static` | 数组和函数都加 `static`，仅限本编译单元使用（不可与 `--header` 同用） |
| `--recursive` | 目录输入时递归查找子目录 |
| `--separator` | 在每个输入前插入 `/* ---- 文件名 ---- */` 注释（默认关闭，保证字符串等于源文件精确拼接） |
| `--crlf` | 把 CRLF 换行统一成 LF |
| `--chunk` | 单个 C 字符串字面量的最大字符数，默认 100，仅影响排版 |

## 4. 示例

### 4.1 单文件、自定义符号名

```bash
python3 js2c.py settings.js -o settings_ui.c --name settings_ui
```

生成 `settings_ui_source()` / `settings_ui_len()`，数组名 `s_settings_ui`。

### 4.2 多文件合并，保持依赖顺序

`ui_common.js` 提供公共函数，必须放在最前面；调用顺序写在命令行里即可：

```bash
python3 js2c.py \
    src/ui_common.js src/launcher.js src/game_2048.js src/main.js \
    -o src/js_bundle.c --header src/js_bundle.h --name js_bundle
```

输出路径不存在时会自动创建目录。

### 4.3 只出数组，嵌入到已有源文件

```bash
python3 js2c.py ui.js -o ui_blob.c --array-only --static --var s_ui_js
```

### 4.4 在 launcher_v9 工程中打包

实际工程的调用方式（在 `projects/lvgl/launcher_v9` 目录下执行）：

```bash
python3 ../../../ap/components/bk_javascript/tools/js2c.py \
    ap/js/ui_common.js ap/js/launcher.js ap/js/game_2048.js \
    ap/js/juice_rush.js ap/js/main.js \
    -o ap/js_bundle.c --header ap/js_bundle.h --name js_bundle
```

这条命令把 5 个脚本按依赖顺序合并成 `ap/js_bundle.c` 与声明头 `ap/js_bundle.h`，
生成物随工程一起编译；修改任意 `.js` 后需要再执行一次，改动才会进入固件。

## 5. 生成代码的约定

- **转义用定宽八进制**（`\344` 这类）而不是十六进制。C 的 `\xNN` 是"最长匹配"，
  `"\x01"` 后面紧跟 `0`~`9`/`a`~`f` 会被合并成另一个字符，存在静默改字节的风险；
  八进制固定 3 位，不会吞掉后续字符。
- 非 ASCII（UTF-8 中文等）和控制字符全部转义，生成文件为纯 ASCII，不受编辑器编码影响。
- 字符串长度用 `sizeof - 1` 计算，不调用 `strlen`，也能容纳内容中的 `\0`。
- 不压缩注释和空白，避免破坏 JS 中的正则字面量与字符串。需要压缩请在 JS 侧先处理。
- 输出确定：相同输入永远生成相同文件，便于版本管理比对。

## 6. 注意事项

- 脚本**没有接入 CMake**，是手动执行的工具，改完 JS 必须手动重跑。
- 单次 `bk_js_eval()` 受 `CONFIG_BK_JERRYSCRIPT_DEFAULT_TIMEOUT_MS` 限制，脚本过大时建议拆分或用 `require()` 分片加载。
- JS 堆大小由 `CONFIG_BK_JERRYSCRIPT_HEAP_SIZE_KB` 决定，内嵌脚本越长，解析期占用越高。
