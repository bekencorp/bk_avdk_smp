# 4K Still Capture Example (shoot4k_example)

* [中文](./README_CN.md)

## 1. Overview

This project demonstrates a 4K single-frame capture flow on the BK7259 platform with the GC8613YA MIPI CSI sensor:

- The sensor outputs 3840x2160 RAW10 and the ISP produces one complete 3840x2160 NV12 frame;
- The 4K NV12 frame can be encoded to JPEG through VCENC;
- The raw JPEG data can be exported to a PC over UART4;
- Three command-line entries are provided: `capture`, `jpeg` and `loop`, and the time cost of each stage is printed.

Since one 4K RAW frame is about 15.8MB and one 4K NV12 frame is about 12.4MB, they cannot coexist in one limited PSRAM slab, so the project uses a three-pass capture flow:

| Pass | Action | Description |
|------|--------|-------------|
| Pass 1 | Open 1920x1080 NV12 @15fps | Wait for one frame, then query the exposure time and analog gain after AE convergence |
| Pass 2 | Open 3840x2160 RAW10 @15fps | Send the exposure seed while the sensor is stopped, borrow one complete 4K RAW frame, then close the channel |
| Pass 3 | Open 1920x2160 NV12 @30fps | Inject the 4K NV12 buffer into the MP channel; RDMA replays the borrowed 4K RAW frame in two halves and the ISP writes directly into the 4K NV12 buffer to build one 3840x2160 NV12 frame |

### 1.1 Reference Environment

- Hardware: BK7259 development board + GC8613YA (MIPI CSI)
- Sensor interface: I2C1, SCL = GPIO_69, SDA = GPIO_70
- Sensor control: XCLK = GPIO_59, RESET = GPIO_71, PWDN not used (0xFF)
- Debug UART: UART0, 115200
- JPEG data port: UART4 (base address `0x458E0000`), 115200 by default
- Flash partitions: 8MB by default (see `partitions/bk7259/auto_partitions.csv`)

> When changing the sensor or the board, the pin definitions in `ap/src/shoot4k_cli.c`, the `CSI_CAM_BUS_I2C1_8BIT_2000TIMEOUT()` bus configuration and `ap/config/bk7259_ap/defconfig` must be updated accordingly.

## 2. Directory Layout

```text
shoot4k_example/
├── .ci                     # CI build entry
├── CMakeLists.txt          # Project-level CMake
├── Makefile                # Project-level Makefile
├── README.md
├── ap/                     # AP side code
│   ├── CMakeLists.txt
│   ├── config/bk7259_ap/   # AP defconfig, default GPIO configuration
│   ├── include/shoot4k_cli.h
│   └── src/
│       ├── ap_main.c       # AP entry: init media service, frame buffer and CLI
│       └── shoot4k_cli.c   # 4K capture, JPEG encoding, UART export and CLI
├── cp/                     # CP side code
│   ├── CMakeLists.txt
│   ├── cp_main.c
│   └── config/bk7259/
└── partitions/bk7259/      # Flash / RAM layout
```

## 3. Build and Flash

### 3.1 Build

Run the following command in the SDK root directory:

```bash
make bk7259 PROJECT=multimedia/shoot4k_example
```

When using the Docker environment:

```bash
./dbuild.sh make bk7259 PROJECT=multimedia/shoot4k_example
```

The same Docker build command is stored in `.ci` in the project root.

The default flash capacity is 8MB. Override it by appending parameters to the command:

```bash
make bk7259 PROJECT=multimedia/shoot4k_example FLASH_CAPACITY=8M PSRAM_CAPACITY=16M
```

After a successful build, `all-app.bin` is generated under `package/` in the build directory (refer to the build log for the actual path).

### 3.2 Flash and Boot

1. Use BKFIL to flash `all-app.bin`; when flashing a new module for the first time, CRC checking must be disabled at the same time. See the Quick Start guide for details.
2. Power cycle the board after flashing.
3. Connect the debug UART on UART0 (115200); after boot, `m55 running...` and the log indicating that the `shoot4k` CLI is registered should be visible.

## 4. CLI Usage

Every command prints `CMDRSP:OK` on success and `CMDRSP:ERROR` on failure. CLI commands are blocking; do not trigger another capture while one is running.

```text
shoot4k fmt [rgb565|yuyv422]    # Show/select the 16bpp output path, rgb565 by default
shoot4k capture                 # Capture one 3840x2160 16bpp frame
shoot4k jpeg                    # Capture one frame and encode it to JPEG
shoot4k loop <n> [jpeg]         # Capture n times, optionally encode JPEG in each round
shoot4k uart_init [baud]        # Initialize the UART4 data port, 115200 by default
shoot4k uart_send jpeg          # Send the latest JPEG over UART4
```

### 4.1 Verify the Capture Path: capture

```text
shoot4k fmt yuyv422
shoot4k capture
```

It only verifies the 4K RAW borrowing, RDMA replay and 16bpp stitching path, and **does not output the image to the serial port or any storage medium**. On success the log contains:

```text
shoot4k: start, loop=1 jpeg=0
shoot_4k: pass1 (AE probe) end ..., cost ...ms
shoot_4k: pass2 (capture, zero-copy) end ..., cost ...ms
shoot4k: total cost ...ms
shoot4k: done, ok 1/1, total ...ms
```

Note: after a successful `capture`, the buffer is kept until the next capture starts, which keeps the interface simple. RGB565/YUYV422 reuse the 16.6MB RAW buffer; NV12 keeps an extra 12.4MB.

### 4.2 Capture and Encode JPEG

```text
shoot4k jpeg
```

The flow is: capture 4K NV12 → encode to JPEG with VCENC → release NV12 and keep only the JPEG. On success the log contains:

```text
jpeg encode done, size=...
jpeg: 0x..., len: ...
shoot4k: done, ok 1/1, total ...ms
```

The JPEG output buffer is a fixed 4MB; encoding fails if a single frame exceeds that size. The JPEG stays in memory until it is overwritten by the next `jpeg` / `loop ... jpeg`, or sent through `uart_send jpeg`.

### 4.3 Continuous Capture

```text
shoot4k loop 5
shoot4k loop 5 jpeg
```

NV12 (and JPEG) is released as soon as each round finishes, so the peak memory is roughly the usage of a single capture. JPEG is not kept in loop mode, therefore `uart_send jpeg` is unavailable after the loop ends (a message is printed in the log).

### 4.4 Export JPEG over UART4

1. Connect the RX of a USB-to-serial adapter to UART4 TX on the board and share the ground;
2. Initialize the data port:

   ```text
   shoot4k uart_init 115200
   ```

3. Capture and encode:

   ```text
   shoot4k jpeg
   ```

4. Send:

   ```text
   shoot4k uart_send jpeg
   ```

5. In the PC serial tool, enable "receive and save to file", keep the baud rate consistent with `uart_init`, and select 8 data bits, no parity, 1 stop bit and no flow control; rename the saved file to `.jpg` to view it.

UART4 carries a raw JPEG byte stream with no extra header or protocol; debug logs are still printed on UART0 and never mix into the JPEG data.

## 5. Capture Flow and Logs

The complete `shoot_4k` flow is as follows (implemented in `isp_shoot_4k_capture()`):

1. **Pass 1 (AE probe)**: open the SP channel at 1920x1080 NV12 @15fps, wait for one frame, read the exposure time and analog gain through `BK_CAM_IOCTL_QUERY_EXPOSURE_INFO`, sample the AWB gains of that frame through `VSI_MPI_ISP_QueryAwbInfo`, then close the channel.
   - A failure to borrow the probe frame only prints a warning and does not affect the following flow; the exposure/AWB information is still queried.
2. **Pass 2 (4K RAW borrowing)**: open the SP channel at 3840x2160 RAW10 @15fps, send the exposure seed obtained in Pass 1 through `BK_CAM_IOCTL_SET_INITIAL_EXPOSURE` before streaming starts, borrow one complete 4K RAW frame and then close the channel.
   - A borrow failure in this pass fails the whole capture.
3. **Pass 3 (RDMA replay and stitching)**: the default output is **16bpp in-place**, with the format selected at runtime by `shoot4k fmt` (RGB565 or YUYV422). The borrowed 4K RAW buffer itself is injected into the MP channel in a zero-copy way, and a 1920x2160 RGB565/YUYV422 @30fps channel is opened; RDMA replays the left/right halves of the RAW frame and the MI writes the left half at `base+0` and the right half at `base+3840` with a 7680B line stride, so after popping two frames the complete 3840x2160 16bpp frame is obtained (16,588,800B, reusing the RAW buffer in place, no extra allocation).
   - RGB565 and YUYV422 are both 2B/px, so a 1920-pixel half-frame line is exactly 3840B, matching one half of the 7680B RAW line.
   - Before Pass 3 streams, the AWB gains sampled in Pass 1 are locked through `BK_CAM_IOCTL_SET_WB` (manual), which is equivalent to one extra AWB iteration and also keeps both halves on the same WB gains.
4. **JPEG encoding (optional)**: `shoot4k_cli.c` calls the low level `JpegEncInit/JpegEncSetPictureSize/JpegEncEncode` directly (`JPEGENC_RGB565` for RGB565, `JPEGENC_YUV422_INTERLEAVED_YUYV` for YUYV422). The output buffer is allocated from the CODED slab first and falls back to UNCODED when it is insufficient.

Timing logs are printed at the end of Pass 1, at the end of Pass 3 and at the end of the command; the unit is milliseconds, which can be used directly to evaluate the cost of each stage.

## 6. Key Configurations

Configurations in `ap/config/bk7259_ap/defconfig` that are directly related to this project:

| Configuration | Purpose |
|---------------|---------|
| `CONFIG_ISP` | ISP driver |
| `CONFIG_MIPI_CSI` | MIPI CSI interface |
| `CONFIG_BK_CAMERA` | Camera framework |
| `CONFIG_CSI_CAMERA` | CSI camera support |
| `CONFIG_CSI_GC8613YA` | GC8613YA sensor driver |
| `CONFIG_FRAME_BUFFER` | Frame buffer / slab management |
| `CONFIG_BK_H264E` | VCENC, required by JPEG encoding |
| `CONFIG_MEDIA_SERVICE` | Media service |

If `CONFIG_BK_H264E` is disabled, `shoot4k jpeg` / `loop ... jpeg` return not supported.

Build switches at the top of `ap/src/shoot4k_cli.c`:

| Macro | Default | Purpose |
|-------|---------|---------|
| `SHOOT4K_MI_Y_LLENGTH_VALUE` | 3840 | MI line-length register value; change to 7680 if the hardware interprets it as bytes |

`partitions/bk7259/ram_regions.csv` defines two large buffers:

- `PSRAM_MEM_SLAB_UNCODED`: 16MB, for uncoded data such as RAW / NV12;
- `PSRAM_MEM_SLAB_CODED`: 14MB, for coded data such as JPEG output.

## 7. Notes

1. **Only a fixed 4K resolution is supported**: the half-frame RDMA replay in Pass 3 is designed for 1920x2160, so changing the output resolution requires updating the parameters and buffer offsets of `isp_func_rdma_write_config()` accordingly.
2. **Upscaling is not supported**: the ISP output resolution must not exceed the sensor output resolution; the sensor format list is checked before opening.
3. **No reentrancy during capture**: the CLI runs the whole capture flow blocking in the calling thread; do not trigger multiple capture commands concurrently.
4. **Peak memory**: 16bpp mode (RGB565/YUYV422) holds only the reused 16.6MB RAW buffer. JPEG mode allocates an extra 4MB output buffer; `loop` mode releases them after each round, which makes it suitable for long-time stress tests.
5. **The 16bpp line stride relies on application-side MI register overrides**: the driver's RGB565/YUYV branches ignore the external stride, so `isp_shoot4k_mi_geometry_override()` writes `MI_MP_Y_LLENGTH/SIZE/PIC_SIZE` right after channel open. If the image is half-width or rows overlap, adjust `SHOOT4K_MI_Y_LLENGTH_VALUE` per the table above.
6. **YUYV422 byte order**: if the YUYV image has wrong colors, change `JPEGENC_YUV422_INTERLEAVED_YUYV` to `JPEGENC_YUV422_INTERLEAVED_UYVY` in `isp_jpeg_encode_yuyv422()` and retest.
7. **In-place reuse has a first-frame hazard**: between the final channel open and the RDMA taking over, a MIPI frame may in theory complete and overwrite the RAW source. If stress tests show random corruption at the top or in the left half, consider setting MI_SKIP until the RDMA is configured.
8. **UART4 is a raw data port**: there is no handshake or retransmission; when transferring large files, lower the baud rate or make sure the serial tool does not drop data.
9. The log TAG is `shoot4k`; filter the capture-related output with this keyword.
