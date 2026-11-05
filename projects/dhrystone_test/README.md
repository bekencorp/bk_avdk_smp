# dhrystone_test example project

* [Chinese](./README_CN.md)

## 1. Overview

This project runs the Dhrystone 2.1 benchmark on the AP core of BK7259 to measure
CPU DMIPS. `CONFIG_DHRYSTONE_TEST` is enabled in the AP defconfig, which registers
the `dhrystone` CLI command.

The project provides:

- The AP command `ap_cmd dhrystone` (dual core in parallel by default, `contexts 2`)
- Clock boost, watchdog handling and a DWT frequency self-test before each run, so
  the measurement is reproducible
- Detailed output: Flash clock, DWT self-test, parallel wall time, DMIPS and
  average cycles/run

The CP side registers `dhrystone` as well; run it directly on the CP serial
port and the rest of the flow is the same.

## 2. Test environment

- Board: BK7259 series or a compatible platform
- Debug UART: 115200 8N1, attached to the CP side (UART0) by default
- Serial terminal: used to read logs and enter CLI commands
- Build environment: `ARMINO_TOOLS_PATH` exported as required by the SDK

## 3. Directory layout

```text
<SDK_ROOT>/
├── projects/dhrystone_test/
│   ├── ap/
│   │   ├── ap_main.c                       # AP entry (calls dhry_cli_init)
│   │   ├── CMakeLists.txt                  # source list and per-file macros
│   │   ├── Kconfig                         # CONFIG_DHRYSTONE_TEST switch
│   │   ├── dhrystone/                      # Dhrystone 2.1 sources, port and CLI command
│   │   │   ├── dhry.h
│   │   │   ├── dhry_1.c / dhry_2.c         # official sources
│   │   │   ├── dhry_1_ctx1.c / dhry_2_ctx1.c  # second instance for the dual-core run
│   │   │   ├── dhry_glue.c                 # timing, printf/scanf redirect, SMP scheduling
│   │   │   ├── dhry_cli.c                  # command registration and pre-run setup
│   │   │   └── dhry_port.h                 # time base, HZ, DMIPS base
│   │   └── config/bk7259_ap/defconfig      # AP config (enables CONFIG_DHRYSTONE_TEST)
│   ├── cp/                                 # CP entry, config and dhrystone sources, same layout
│   ├── partitions/bk7259/
│   │   ├── auto_partitions.csv
│   │   └── ram_regions.csv
│   ├── CMakeLists.txt
│   ├── Makefile
│   ├── pj_config.mk
│   └── README.md / README_CN.md
```

The DMIPS sources, porting layer and CLI command all live with the project under
`projects/dhrystone_test/ap/dhrystone/` and `cp/dhrystone/`; `bk_cli` no longer
builds Dhrystone or registers its command.

## 4. Build and flash

Run in the SDK root:

```bash
cd <SDK_ROOT>
export ARMINO_TOOLS_PATH=$PWD/tools/build_tools
make bk7259 PROJECT=dhrystone_test
```

Build outputs:

| File | Description |
| --- | --- |
| `build/bk7259/dhrystone_test/package/all-app.bin` | firmware image to flash |
| `build/bk7259/dhrystone_test/bk7259_ap/app.elf` | AP core ELF, for map/symbol analysis |

## 5. Usage

### 5.1 Run the AP-side test

Type in the CP serial console:

```text
ap_cmd debug dhrystone              # 5,000,000 runs by default
ap_cmd debug dhrystone 20000000     # custom run count
ap_cmd debug help                   # list all AP-side debug sub-commands
```

### 5.2 Environment handling around the run

Before running, the AP side performs these steps in order:

1. Stop WWDT/TaskWDT
2. Block the idle sleep vote
3. Raise the CPU clock to 480MHz
4. Print the Flash clock and run the DWT frequency self-test
5. Run inside dedicated dhrystone threads (one instance on core 0 and one on
   core 1 by default, 4096-byte stack each)
6. Restore the 480MHz setting, the sleep vote and the watchdogs

This prevents the CPU clock from being gated or scaled down and the measurement
from being preempted by other tasks. The system state is the same before and
after the test, so later functions are unaffected.

Dual core in parallel is the default (`contexts 2`); the second independent
instance is produced by `dhry_1_ctx1.c` / `dhry_2_ctx1.c`, which `#include` the
official sources and rename the symbols. The official sources stay unmodified.

### 5.3 Run the CP-side test (optional)

Type `dhrystone` directly on the CP serial port; the clock boost, watchdog
and timing flow are handled by the CP side.

## 6. Reading the output

```text
Flash clk: cksel=... ckdiv=... => source=... div=/...
DWT self-test 100ms: DWT=... cy (0.100s@480M)  Timer0=... tk (0.100s@26M)  ratio=1.00
Dhrystone 2.1, runs 5000000, SMP 2 cores (ctx0 on AP core 0, ctx1 on AP core 1)
Config: dhrystone -Os, code FLASH-XIP, data SRAM-HEAP, 480 MHz, contexts 2
[c0] Execution starts, 5000000 runs through Dhrystone
[c1] Execution starts, 5000000 runs through Dhrystone
...
[c0] Final values of the variables used in the benchmark:
...
Parallel 2 cores: 2 x 5000000 runs in 15.612 s wall
Dhrystones per Second (2 cores): 640533.0
DMIPS (2 cores): 364.56   DMIPS/MHz (2 cores): 0.759   avg cycles/run: 1502
```

- `Flash clk`: Flash clock source and divider
- `DWT self-test`: DWT cycle count cross-checked against the 26MHz Timer0; a
  `ratio` close to 1.00 confirms the core really runs at 480MHz, otherwise the
  score is not trustworthy
- `Config`: compile options, code/data placement and frequency of this run
- `Parallel 2 cores: ... runs in ... s wall`: the whole-chip parallel window; it
  starts before the threads are created and stops after the last core finishes
- `Dhrystones per Second (2 cores)`: the whole-chip score, i.e.
  **total work / parallel wall time** = `contexts × runs` ÷ window, not a sum of
  the per-core rates
- `DMIPS (2 cores)`: whole-chip Dhrystones/s ÷ 1757
- `DMIPS/MHz (2 cores)`: whole-chip DMIPS ÷ 480MHz, for comparison across
  different CPU frequencies
- `avg cycles/run`: mean cycles per run of the two cores, used to locate
  fetch/memory bottlenecks; not part of the reported score
- `[c0]` / `[c1]`: output prefix of the two instances, only used to confirm both
  cores finished and every Final value matches its `should be:` counterpart

DMIPS calculation:

```text
Dhrystones per Second = (contexts × runs) ÷ parallel wall time (s)
DMIPS                 = Dhrystones per Second ÷ 1757
DMIPS/MHz             = DMIPS ÷ 480
```

1757 is the Dhrystones/s of a VAX 11/780, defined as 1 DMIPS; `contexts` is the
number of instances (2 by default) and `runs` is the run count per instance.

The official program also prints `Final values of the variables used in the
benchmark`. Every value must match its `should be:` counterpart; otherwise the
port has broken the arithmetic semantics and the result is not trustworthy.

## 7. Related configuration and code

| Location | Description |
| --- | --- |
| `CONFIG_DHRYSTONE_TEST` | Kconfig switch in `projects/dhrystone_test/ap/Kconfig` (`cp/Kconfig` for CP), disabled by default |
| `projects/dhrystone_test/ap/dhrystone/` | official sources + porting layer (`cp/dhrystone/` for CP) |
| `projects/dhrystone_test/ap/dhrystone/dhry_port.h` | time base, HZ, DMIPS base, default run count |
| `projects/dhrystone_test/ap/dhrystone/dhry_cli.c` | `dhrystone` command registration and pre-run setup (clock boost, watchdogs) |
| `projects/dhrystone_test/ap/CMakeLists.txt` | source list, per-file macros and compile options |

To disable the command, remove `CONFIG_DHRYSTONE_TEST=y` from the project defconfig
(or set `# CONFIG_DHRYSTONE_TEST is not set`) and rebuild.

## 8. Quick start

### 8.1 Code configuration

Data placement and optimization level are configured with environment variables
before building and read by `projects/dhrystone_test/ap/CMakeLists.txt`; Flash cache is
configured in the project defconfig.

**Data placement**

Location of the two `Rec_Type` records; the default is on-chip SRAM (SRAM5/SRAM6,
starting at 0x2c180000):

| Data placement | Configuration |
| --- | --- |
| on-chip SRAM (default) | do not set any `DHRYSTONE_*_DATA` |
| high-speed SRAM3 (0x2c100000) | `DHRYSTONE_SRAM3_DATA=1` |
| high-speed SRAM4 (0x2c140000) | `DHRYSTONE_SRAM4_DATA=1` |
| DTCM | `DHRYSTONE_DTCM_DATA=1` |

**Optimization level**

Controlled by `DHRYSTONE_OPT_FLAG`, default `-Os`, can be changed to `-O3`.

**Flash cache**

Defaults to L1 I-cache on, L1 D-cache off, L2 cache on. To change it, edit
`ap/config/bk7259_ap/defconfig`:

```text
CONFIG_ICACHE=y
CONFIG_DCACHE=n
CONFIG_L2_CACHE_ENABLE=y
```

Rebuild and flash after the change.

### 8.2 Build and flash

1. Build command
`make bk7259 PROJECT=dhrystone_test`
2. Flash image
`build/bk7259/dhrystone_test/package/all-app.bin`

### 8.3 AP-side Dhrystone test

```text
# AP side, dual core in parallel, 5,000,000 runs
ap_cmd dhrystone

# Increase the run count if the measurement window is too small
ap_cmd dhrystone 20000000
```

### 8.4 Reading the result

After confirming the `ratio` of `DWT self-test` is close to 1.00 and every
`Final values` entry matches `should be:`, read the last line:

```text
DMIPS (2 cores): 364.56   DMIPS/MHz (2 cores): 0.759   avg cycles/run: 1502
```

## 9. Reference test data

The table below lists measured data on BK7259 (Cortex-M55, dual core at 480 MHz)
with code in Flash, L1 I-cache on, L1 D-cache off and L2 cache on (the repository
default). It is meant for comparing data placements and optimization levels, not
as an absolute score for other boards or firmware versions.

| Data placement | Optimization | DMIPS  | DMIPS/MHz |
| ---            | ---          | ---    | ---       | 
| heap           | -Os          | 357.68 | 0.745     |
| heap           | -O3          | 600.84 | 1.252     |
| SRAM3          | -Os          | 484.32 | 1.009     |
| SRAM3          | -O3          | 646.07 | 1.346     |
| DTCM           | -Os          | 382.14 | 0.796     |
| DTCM           | -O3          | 723.28 | 1.507     |
