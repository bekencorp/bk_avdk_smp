# GPU 2.5D Tiger Example

* [中文](./README_CN.md)

## 1. Overview

`gpu_2.5D_tiger` is a GPU 2.5D graphics display example for BK7259. It demonstrates the complete display path that combines a MIPI DSI LCD, DPU, frame buffer and the VG-Lite GPU. After boot, the firmware initializes the display device, renders a vector tiger on the LCD and cycles through scale, rotate and translate animations.

This example demonstrates:

- MIPI DSI LCD panel initialization and open flow.
- DPU ARGB8888 layer refresh flow.
- VG-Lite GPU vector path rendering APIs.
- Double frame-buffer display switching.
- Synchronization between the DPU release callback and the render thread.
- Slab heap allocation for GPU render buffers.

## 2. Hardware Requirements

- SoC/board: BK7259 series development board.
- Display: MIPI DSI LCD. The default panel is `lcd_device_hx8399c_mipi_1080x1920`.
- Memory: board-side PSRAM must be available. Display buffers and GPU contiguous memory depend on PSRAM resources.
- Debug interface: UART console for startup and runtime logs.

Default pin assignment:

- LCD reset: `GPIO_60`.
- LCD backlight: `GPIO_7`.

If the actual hardware uses a different panel or different pins, update the LCD driver, panel timing, initialization sequence and GPIO configuration accordingly.

## 3. Project Structure

```text
gpu_2.5D_tiger/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── ap_main.c          # AP entry, media service, monitor and tiger demo startup
│   ├── draw_tiger.c       # LCD/DPU/GPU initialization and animation rendering
│   ├── draw_tiger.h
│   ├── tiger_paths.h      # Vector tiger path and color data
│   └── config/
├── cp/
│   ├── cp_main.c
│   └── config/
└── partitions/
```

## 4. Build And Flash

Run the build command from the SDK root directory:

```bash
make bk7259 PROJECT=multimedia/gpu_2.5D_tiger -j
```

After the build completes, the firmware image is generated at:

```text
build/bk7259/gpu_2.5D_tiger/package/all-app.bin
```

Flash `all-app.bin` to the board. After flashing, reset the board and open the UART terminal to check logs.

## 5. Expected Result

The example runs automatically after boot. No CLI command is required. The startup flow is:

1. Initialize the system and media service.
2. Start the AVDK monitor.
3. Initialize the MIPI DSI bus, LCD panel and DPU.
4. Initialize the VG-Lite GPU.
5. Create the `gpu` render thread.
6. Switch between two frame buffers and refresh the display continuously.

UART logs should contain output similar to:

```text
M55 main running...
draw_tiger
read lcd id: 0x...
render_tiger_task
tiger animation round complete
```

The LCD should show a vector tiger on a purple background. The animation cycles through:

- Scale: the tiger zooms in and out.
- Rotate: the tiger rotates around the center.
- Translate: the tiger moves within the screen area.

## 6. Key Configuration

This example depends on the following main configuration options:

```text
CONFIG_BK_DISPLAY=y
CONFIG_MIPI_DSI=y
CONFIG_DPU_DRIVER=y
CONFIG_DSI_DRIVER=y
CONFIG_FRAME_BUFFER=y
CONFIG_VG_LITE_GPU=y
CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ=0x3A000
CONFIG_LCD_HX8399C_MIPI_1080x1920=y
CONFIG_MEDIA_SERVICE=y
```

When adapting another LCD, check:

- The LCD driver enabled in `ap/config/bk7259_ap/defconfig`.
- The LCD header included by `ap/draw_tiger.c`.
- The `lcd_device_*` descriptor used by `ap/draw_tiger.c`.
- LCD reset and backlight GPIOs.
- Frame-buffer width, height, stride and pixel format.

## 7. Notes

1. The default panel is `hx8399c_mipi_1080x1920`. Other panels usually require matching timing and initialization sequences.
2. GPU rendering requires contiguous memory. If frame-buffer or VG-Lite initialization fails, check PSRAM and `CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ` first.
3. The example uses double buffering. The render thread waits for the DPU release callback before reusing a buffer.
4. This example starts automatically after boot and does not provide additional CLI test commands.

## 8. Troubleshooting

### No Display

Check LCD power, backlight, MIPI DSI cable, reset GPIO and backlight GPIO. If the UART log does not show `read lcd id` or display initialization messages, check panel creation and DSI bus initialization first.

### Corrupted Image Or Wrong Position

Make sure the actual LCD model and resolution match `lcd_device_hx8399c_mipi_1080x1920`. A different panel requires matching panel timing, initialization sequence and frame-buffer parameters.

### GPU Initialization Fails

Make sure `CONFIG_VG_LITE_GPU` is enabled, then check contiguous memory size and PSRAM initialization status.
