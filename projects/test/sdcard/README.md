# BK7259 SD Card Test

English | [中文](README_CN.md)

This project is derived from `projects/app` and contains no solution-layer
code. It provides one `sdcard` CLI command covering FATFS mount/unmount,
read/write integrity tests, throughput benchmarks, random I/O, soak, and a
background PSRAM/CPU/bus load generator for testing SDIO under load.

## Hardware

- BK7259 board with SDIO0 one-line (GPIO2/3/4) or SDIO1 four-line
  (GPIO14~GPIO19).
- A FAT-formatted SD card.
- Serial console at 115200 baud for the `ap_cmd` shell.

The SDIO host and GPIO mapping are selected in
`ap/config/bk7259_ap/defconfig`:

- SDIO0 one-line: CLK/CMD/DAT0 on GPIO2/GPIO3/GPIO4
- SDIO1 four-line: CLK/CMD/DAT0~DAT3 on GPIO14~GPIO19
- SD-NAND power enable: GPIO53, active high (SDIO1 mode only)
- AP CPU frequency vote: 480MHz
- FATFS volume: `1:` (`sdio_sd`)

## Build

Run from the SDK root:

```sh
make clean PROJECT=test/sdcard
make bk7259 PROJECT=test/sdcard
```

The firmware is generated at:

```text
build/bk7259/sdcard/package/all-app.bin
```

## SDIO host selection

SDIO0, one-line (compatible with the `app` project and does not occupy the
UART0-shared SDIO0 DAT1~DAT3 pins):

```text
CONFIG_SDCARD_HOST_ID=0
CONFIG_SDCARD_BUSWIDTH_4LINE=n
```

SDIO1, four-line (Beken Robot SD-NAND):

```text
CONFIG_SDCARD_HOST_ID=1
CONFIG_SDCARD_BUSWIDTH_4LINE=y
```

`usr_gpio_cfg.h` selects the matching pin table at compile time and rejects
unsupported host/bus-width combinations.

## Speed modes

Edit `ap/config/bk7259_ap/defconfig` before each clean build.

Default Speed, 20MHz:

```text
CONFIG_SDCARD_HIGH_SPEED=n
CONFIG_SDCARD_CLOCK_FREQ_HZ=20000000
```

High Speed, 40MHz:

```text
CONFIG_SDCARD_HIGH_SPEED=y
CONFIG_SDCARD_CLOCK_FREQ_HZ=40000000
```

## Commands

All commands are run on the AP CLI:

```text
ap_cmd sdcard <sub command>
```

| Command | Description |
|---|---|
| `sdcard mount` / `sdcard umount` | Mount or unmount `1:` and report FRESULT plus the time taken |
| `sdcard card` | Card type, capacity, state and current SDIO configuration |
| `sdcard info` | Mounted state, total/free/used space and cluster size |
| `sdcard write <file> <size_kb> [sync] [seed]` | Write-only test; `sync=1` calls `f_sync` after every 64KB chunk |
| `sdcard read <file> [size_kb] [seed]` | Read-only test; omit `size_kb` (or use 0) to read the whole file; with `seed`, verifies the data written by `write` |
| `sdcard rwtest [size_kb] [rounds]` | Write + read-back + verify loop, temp file deleted afterwards |
| `sdcard randrw [size_kb] [chunk_kb] [rounds]` | Random-offset write + read-back + verify, reports MB/s and IOPS |
| `sdcard mounttest [rounds] [delay_ms]` | Repeated mount/unmount with per-round timing and pass/fail counters |
| `sdcard bench [size_mb] [rounds] [drive_capacity]` | Sequential write/read throughput benchmark |
| `sdcard soak start [size_kb] [duration_s]` | Background write/read/verify soak; `0` means run until stopped |
| `sdcard soak stop` / `sdcard soak status` | Stop the soak or print its counters |
| `sdcard load start [work_kb] [mode] [chunk_kb]` | Start PSRAM/CPU/bus load; mode `1=copy 2=fill 3=read` |
| `sdcard load stop` / `sdcard load status` | Stop or inspect the load generator |
| `sdcard loadtest [size_mb] [rounds] [work_kb]` | One-key: start load, run bench, run a 1MB verify, stop load |
| `sdcard status` | Mount, load and soak state |

### Defaults and limits

| Command | Defaults / limits |
|---|---|
| `write` | `size_kb` required, 1..65536; `sync` 0/1 default 0; `seed` default 0x5A |
| `read` | `size_kb` optional, 0..65536 (0 or omitted = whole file); `seed` optional but requires `size_kb` |
| `rwtest` | `size_kb` 1024, `rounds` 1; size 1..65536, rounds 1..100 |
| `randrw` | 1024KB / 4KB / 1 round; size 1..65536, chunk 1..64KB, rounds 1..100; chunk ≤ size |
| `mounttest` | 10 rounds, 100ms delay; rounds 1..1000, delay 0..10000ms |
| `bench` | 16MB, 3 rounds; size 1..64MB, rounds 1..10; capacity default from the host configuration |
| `soak` | 1024KB, 60s; size 1..65536KB, duration 0..86400s |
| `load` | 128KB copy, 64KB chunk; work 1..192KB |
| `loadtest` | 16MB, 3 rounds, 128KB load work |

### Command behavior notes

- `read` verification: use `sdcard read <file> <size_kb> <seed>` after
  `sdcard write <file> <size_kb> <sync> <seed>` with the same size and seed.
  The seed is positional, so `size_kb` cannot be omitted when `seed` is used.
- `mounttest` starts by unmounting, then performs a real mount + unmount per
  round and leaves the volume unmounted.
- `soak` runs in a background thread. While it is running, other FS commands
  (`mount/umount/info/write/read/rwtest/randrw/mounttest/bench/loadtest`) are
  rejected; `soak stop` exits after the current 64KB chunk.
  `card`, `status` and the `load` commands remain available.
- FS commands (`card/info/write/read/rwtest/randrw/bench/soak start/loadtest`)
  auto-mount `1:` and leave it mounted; use `sdcard umount` to release it.
  `mounttest` starts by unmounting and ends unmounted.
- Do not mix `fatfstest M/U` with `sdcard` because both keep their own FATFS
  object on the same volume. `fatfstest F 1` can still be used to format a
  card before the first mount.
- Temporary files: `1:/sdtest_bench.bin`, `1:/sdtest_rw.bin`,
  `1:/sdtest_rand.bin`, `1:/sdtest_soak.bin`. They are unlinked when the
  corresponding test finishes.

## Typical flows

Basic integrity and throughput:

```text
ap_cmd sdcard mount
ap_cmd sdcard info
ap_cmd sdcard rwtest 1024 3
ap_cmd sdcard bench 16 3
ap_cmd sdcard umount
```

Pure write and pure read, including cache-flush impact:

```text
ap_cmd sdcard write test.bin 1024 0 90
ap_cmd sdcard read test.bin 1024 90
ap_cmd sdcard write test.bin 1024 1 90
ap_cmd sdcard read test.bin 1024 90
```

Mount/unmount robustness:

```text
ap_cmd sdcard mounttest 10 100
```

SDIO under CPU/bus load:

```text
ap_cmd sdcard load start 128 1 64
ap_cmd sdcard load status
ap_cmd sdcard bench 16 3
ap_cmd sdcard rwtest 1024 1
ap_cmd sdcard load stop
```

One-key load test:

```text
ap_cmd sdcard loadtest 16 3 128
```

Long-running reliability, optionally under load:

```text
ap_cmd sdcard load start 128 1 64
ap_cmd sdcard soak start 1024 600
ap_cmd sdcard soak status
ap_cmd sdcard soak stop
ap_cmd sdcard load stop
```

All results are printed with an `[SDTEST]` prefix (the internal module tag);
throughput records include `load=on/off` so loaded and unloaded results can
be compared directly.
