# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Repository-wide behavior rules live in `/CLAUDE.md` (one level up, at the workspace root) — that file (mirrored in `AGENTS.md`) defines the mandatory Define/Plan/Build/Verify/Review/Ship workflow, minimal-diff discipline, and communication standards. Follow it for every task in this repo; this file only adds SDK-specific facts.

## What this is

Armino SMP — Beken's open-source AIoT audio/video and smart-terminal SDK for the **BK7259** chip. Provides Wi-Fi/BLE/Bluetooth/Thread stacks, audio/video codecs, LCD display, on-device AI inference (TFLite Micro), and low-power management, plus example applications under `projects/`.

Full docs: https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/zh_CN/v4.0.1/ (also see `BK7259 Datasheet_V1.3.pdf` at the workspace root).

## Architecture: AP + CP dual-image system

BK7259 has two independent CPU subsystems, each with its own component tree, middleware, and build:

- **CP** (`cp/`) — runs on CPU0, optionally CPU0+CPU1 SMP. Hosts Wi-Fi/BLE/Bluetooth stacks, low-power keepalive, power management, connectivity.
- **AP** (`ap/`) — runs on CPU2+CPU3 SMP (default dual-core). Hosts multimedia (camera/display/H.264/JPEG/GPU/VPU), the audio framework, LVGL GUI, application logic, on-device AI.

The two sides build as separate firmware images and communicate via Mailbox/shared memory. A packaging step combines both into one flashable `all-app.bin`. Never assume a component in `ap/components/` and one with the same name in `cp/components/` are the same code — they are separate trees (e.g. both have `bk_wifi`, `bk_common`, `bk_cli`, etc. with independent implementations).

Each project under `projects/<name>/` mirrors this split:
```
projects/<name>/
  ap/            # AP-side app source + CMakeLists.txt
  cp/            # CP-side app source + CMakeLists.txt
  ap/config/<soc>/   # AP Kconfig defconfig
  cp/config/<soc>/   # CP Kconfig defconfig
  partitions/<soc>/  # flash partition CSVs, RAM region CSVs
  Makefile       # thin wrapper including tools/build_tools/build_files/project_main.mk
  CMakeLists.txt # thin wrapper including project.cmake
```

## Build commands

Build from the SDK root (this directory), not from inside a project:

```bash
make bk7259 PROJECT=app          # build AP+CP in parallel, package firmware (PROJECT defaults to "app")
make bk7259_ap PROJECT=app       # build AP side only
make bk7259_cp PROJECT=app       # build CP side only
make bk7259_ap_menuconfig PROJECT=app   # AP Kconfig menuconfig
make bk7259_cp_menuconfig PROJECT=app   # CP Kconfig menuconfig
make clean
make help
```

Docker build (image `bekencorp/armino-idk`, tag `1.5`+): prefix any make invocation with `./dbuild.sh` (Linux/macOS) or `.\dbuild.ps1` (Windows PowerShell), e.g. `./dbuild.sh make bk7259 PROJECT=app`.

Firmware output: `build/bk7259/<project>/package/all-app.bin` (relative to SDK root).

Useful build-time variables:
- `FLASH_CAPACITY=8M|16M` — selects `partitions/<soc>/auto_partitions[_16M].csv`; 16M output goes to `build/bk7259/<project>_16M/` so it doesn't collide with the 8M build. Auto-detected from `CONFIG_FLASH_CAPACITY_16M=y` in the CP defconfig if not passed explicitly.
- `PSRAM_CAPACITY=16M|32M` — selects `ram_regions[_32M].csv`; independent of `FLASH_CAPACITY`, appends `_psram32M` to the output dir. Auto-detected from `CONFIG_PSRAM_CAPACITY_32M=y` in the CP defconfig.
- `BUILD_DIR=<path>` — override output root (default `<sdk_root>/build`).
- `BK_CONFIG_FILE=<name>` — substitute an alternate `.config` file, appends `_<name>` to the project build name.

AP and CP build in parallel with fail-fast via `tools/build_tools/build_smp_parallel.sh` on Linux/macOS/Docker; Windows falls back to sequential `make -j`.

Secure firmware (`CONFIG_SECURITY_FIRMWARE=y` in the project's `config/<soc>/config`) is packed by TF-M/MCUBoot via a board-specific `.wrapper` script instead of the generic SMP packager — the normal `aboot`/`arm_bootloader` bootloader build is skipped in that case.

Docs generation: `make doc` (runs `smp_doc`, `ap_doc`, `cp_doc` in parallel); `make ap_doc` / `make cp_doc` / `make smp_doc` individually.

## Flashing

Besides the BKFIL GUI/web tools in `README.md`, this machine has the **`bk_loader`** CLI on PATH (`/usr/local/bin/bk_loader`) for flashing over UART:

```bash
bk_loader download -p /dev/tty.usbserial-XXXX -i build/bk7259/<project>/package/all-app.bin -r
```

- `-p/--portnum` serial device (macOS: `/dev/tty.usbserial-*` or `/dev/cu.usbserial-*`); `-b/--baudrate` default `2000000`; `-r` reboot after download; `-e/--pre_erase 1` erase before write.
- Other subcommands: `erase`, `read`, `batch_download`, `secure`, `inject_key`, `authorize`. Run `bk_loader <cmd> --help` for options. Flashing touches live hardware — confirm the port and the target `.bin` before erasing or downloading.

## Component conventions

Every component directory has a `CMakeLists.txt` registering it via `armino_component_register()` with explicit `SRCS`, `INCLUDE_DIRS`, and `REQUIRES` (dependency list) — check an existing component's `CMakeLists.txt`/`Kconfig` before adding files or dependencies to another. Public headers for a component typically live under its own `include/` subdirectory.

Kconfig options are declared per-component (`Kconfig` file) and selected per-project via `ap/config/<soc>/defconfig` or `cp/config/<soc>/defconfig`; use `menuconfig` (see above) rather than hand-editing defconfig files unless you know the exact symbol name.

## Testing

- **`tools/dump_test/`** — standalone Python 3.8+ host-side test suite (stdlib only) that drives real hardware over a serial log tool to validate AP/CP memory-dump and heartbeat-owner/follower behavior. Has its own detailed `README.md` covering safety config, fault-injection modes, and pass/fail semantics — read it before touching this tool, since dump/flash test cases interact with live hardware and flash regions. Self-test: `python3 tests/test_dump_integrity.py -v`.
- **`ap/components/unity`** — Unity C test framework, available as a component for on-target unit tests within AP firmware.
- Project-level `.ci/` directories (e.g. `projects/app/.ci/`) hold CI-specific fixtures per project.

There is no single top-level `make test`; verification is project- and component-specific — check for a `.ci/` directory or component-local tests before assuming none exist.
