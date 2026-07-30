# QR Code Recognition Example

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates camera-based QR code recognition on the Beken platform. It reads `640x360` NV12 frames from the ISP, decodes QR codes from the Y plane using ZBar, and prints the location, payload, and scan time. It can also preview the camera stream on a MIPI panel through the GPU.

The project provides:

- SP recognition with MP display preview
- MP-only recognition without GPU or display
- Optional QR Wi-Fi provisioning after recognition
- Up to four QR code results per frame
- Logs for corner coordinates, payload, and scan time

### 1.1 Test Environment

- Core board: `BK7259_QF128_12.3X12.3_V4.0`
- PSRAM: 32 MB
- Camera: CSI GC2053, `1280x720@30fps` by default
- Display: ER68576B MIPI DSI panel, `720x1280`
- Recognition input: `640x360` NV12; ZBar uses the Y plane
- Graphics: VG-Lite GPU and DPU/MIPI

> Use the reference peripherals where possible. When changing the sensor, panel, or GPIO assignment, update the board configuration in `ap_main.c` and the project Kconfig.

## 2. Directory Structure

```text
qr_example/
├── .ci                         # CI build command
├── app.rst                     # Documentation skeleton
├── CMakeLists.txt              # Project CMake configuration
├── Makefile                    # Make build entry
├── README.md                   # English documentation
├── README_CN.md                # Chinese documentation
├── ap/
│   ├── ap_main.c               # AP initialization and board configuration
│   ├── CMakeLists.txt
│   ├── config/                 # AP default configuration
│   ├── include/
│   └── src/qr_demo.c           # ZBar recognition and CLI implementation
├── cp/                         # CP startup and calibration code
└── partitions/                 # Partition and RAM-region configuration
```

## 3. Feature Description

### 3.1 `start` Preview Mode

1. Start the MIPI sensor and ISP MP channel
2. Create a `640x360` NV12 SP channel for recognition
3. Start the MIPI display, GPU, and ISP-GPU bond for MP preview
4. Read SP frames and pass the NV12 Y plane to ZBar
5. Wait 100 ms between scans
6. Print the count, corners, payload, and scan time after successful recognition

### 3.2 `start_mp` MP-only Mode

This mode temporarily configures ISP MP as `640x360` NV12 and recognizes QR codes directly from MP frames. It does not start SP, GPU, or the display. The original camera configuration is restored by `stop`.

### 3.3 Recognition Parameters

- Input size: `640x360`
- Input format: NV12; the first `640x360` bytes form the Y plane used by the decoder
- Maximum results per frame: 4
- Payload buffer per result: 8,896 bytes
- Scan interval: 100 ms

### 3.4 Optional QR Wi-Fi Provisioning

`start wifi` and `start_mp wifi` keep QR recognition running and additionally parse recognized payloads as Wi-Fi provisioning data. A valid payload must be a JSON object with exactly these three string fields:

```json
{"p":"12345678","s":"test_wifi","t":"1234512345"}
```

- `s`: Wi-Fi SSID, 1 to 32 bytes
- `p`: Wi-Fi password, empty is allowed; non-empty passwords must be 8 to 63 bytes
- `t`: token/filter field, currently only required to exist

After one valid payload starts a Wi-Fi connection, later scans continue to be printed but will not start Wi-Fi again while the local QR Wi-Fi state is connecting, connected, or already has an IP. A first scan miss (`WIFI_REASON_NO_AP_FOUND`) is kept as `CONNECTING` because the Wi-Fi stack may continue with a full-channel retry. When `EVENT_NETIF_GOT_IP4` arrives, the demo records and prints the assigned IP and moves to `GOT_IP`.

## 4. Build and Run

### 4.1 Build

Run from the SDK root:

```bash
make bk7259 PROJECT=multimedia/qr_example
```

### 4.2 CLI Commands

After flashing and booting the firmware, use the serial console:

```text
ap_cmd qr help
ap_cmd qr start
ap_cmd qr start wifi
ap_cmd qr start_mp
ap_cmd qr start_mp wifi
ap_cmd qr stop
ap_cmd qr stop_mp
ap_cmd qr wifi status
ap_cmd qr wifi disconnect
```

- `start`: starts SP QR recognition with MP display preview
- `start wifi`: starts SP recognition and enables QR Wi-Fi provisioning
- `start_mp`: starts MP-only recognition without display
- `start_mp wifi`: starts MP-only recognition and enables QR Wi-Fi provisioning
- `stop`: stops recognition and releases camera, ZBar, GPU, display, and frame-buffer resources
- `stop_mp`: MP-only stop alias; releases the same QR resources as `stop`
- `wifi status`: prints local QR Wi-Fi state, current STA link status, RSSI, and the recorded IP
- `wifi disconnect`: stops Wi-Fi STA and resets the QR Wi-Fi state
- `CMDRSP:OK` indicates success; `CMDRSP:ERROR` indicates invalid arguments or a resource startup failure

## 5. Test Examples

### 5.1 Recognition with Preview

```text
ap_cmd qr start
```

Place a QR code in the camera view. Expected logs include:

```text
frame=... found 1 QR code(s), scan=... us
QR[0] corners=(...,...),(...,...),(...,...),(...,...)
QR[0] len=... data=...
```

Stop the test with:

```text
ap_cmd qr stop
```

### 5.2 MP-only Recognition

```text
ap_cmd qr start_mp
```

This mode has no display preview, but recognition results are still printed to the serial console. Run `ap_cmd qr stop` when finished.

### 5.3 Wi-Fi Provisioning

```text
ap_cmd qr start wifi
```

Show a QR code containing the JSON provisioning payload. The demo prints all QR recognition results and starts Wi-Fi only when the payload passes the `p/s/t` format and length checks. Use `ap_cmd qr wifi status` to inspect the local provisioning state, Wi-Fi STA link state, RSSI, and IP, or `ap_cmd qr wifi disconnect` to stop the STA connection.

Expected Wi-Fi provisioning logs include:

```text
QR Wi-Fi state: IDLE -> CONNECTING
QR Wi-Fi connected: ssid=...
QR Wi-Fi state: CONNECTING -> CONNECTED
QR Wi-Fi got IP: if=... ip=...
QR Wi-Fi state: CONNECTED -> GOT_IP
```

## 6. Configuration

The default configuration enables ISP, MIPI CSI, frame buffers, VG-Lite GPU, DPU, MIPI DSI, GC2053, ER68576B, media service, CJSON, Wi-Fi VNET controller, BK netif, and LWIP. Main board parameters in `ap/ap_main.c` include:

- Sensor GPIOs, I2C, resolution, and frame rate
- ISP MP size and format
- MIPI panel type and GPIOs
- GPU formats and 90-degree rotation

Recognition size, result count, and scan interval are defined by the `QR_*` macros in `ap/src/qr_demo.c`. The ZBar implementation is provided by the project's `zbar` module dependency.

## 7. Notes

1. `start` and `start_mp` cannot run together; repeated starts return busy.
2. `start` occupies camera, ISP, GPU, DPU, display, and frame-buffer resources. Run `ap_cmd qr stop` before switching modes.
3. ZBar reads only the NV12 Y plane. If the recognition size changes, update the ISP output as well.
4. QR codes should be sharp, complete, and high-contrast. Glare, defocus, motion blur, and very small codes reduce recognition reliability.
5. `CMDRSP:OK` only confirms that the task started. Successful recognition is indicated by the `found ... QR code(s)` log.
6. `start wifi` and `start_mp wifi` call `bk_wifi_sta_stop()` before applying a new QR Wi-Fi config, so repeated provisioning starts from a clean STA state.
7. A temporary `WIFI_REASON_NO_AP_FOUND` during `CONNECTING` is treated as a scan miss, not final failure; wrong password or later disconnect events still move the local state to `FAILED`.
