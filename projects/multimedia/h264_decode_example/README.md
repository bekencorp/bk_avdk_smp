# H264 Decoding Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates whole-frame H264 decoding on the Beken platform using the
**`bk_decoder`** stack. It exercises three `bk_h264_decode_ctlr` controllers and
embeds two 1280x720 streams (a non-B 1I30P stream and a B-frame IBBP stream).

Everything is exposed through the single `h264_decode` CLI:

| Controller | CLI | B-frames | Compatible stream |
|------------|-----|----------|-------------------|
| frame (whole-frame) | `h264_decode vcdec_h264d [1280x720_1i30p\|1280x720_ibbp]` | No | `1280x720_1i30p` |
| flexa (segmented) | `h264_decode vcdec_h264d_flexa [1280x720_1i30p\|1280x720_ibbp]` | No | `1280x720_1i30p` |
| frame-zerocopy (zero-copy) | `h264_decode vcdec_h264d_frame_zerocopy [1280x720_1i30p\|1280x720_ibbp]` | Yes | `1280x720_1i30p` and `1280x720_ibbp` |

- When no stream argument is given, the default stream is `1280x720_ibbp`; `1280x720` is
  accepted as an alias for it.
- The frame and flexa controllers are non-B; use them only with `1280x720_1i30p`. The
  frame-zerocopy controller is the only one that decodes the B-frame stream (`1280x720_ibbp`).
- When `CONFIG_BK_DECODER` is enabled (on by default in `defconfig`), `main()` starts a boot
  self-test demo (`vcdec_h264_run_boot_demo()`) that runs frame(1i30p), flexa(1i30p) and
  frame-zerocopy(ibbp) once each in a dedicated worker thread.

* Developer guide: [H264 Decoding (SW) Overview](../../../developer-guide/video_codec/h264_decoding_sw.html)

### 1.1 Test Environment

- Hardware: **BK7259_QF128_12.3X12.3_V4.0**, PSRAM 32M
- Input: built-in H264 streams (`1280x720_1i30p`, `1280x720_ibbp`)
- Output: decode logs and `[RESULT][PASS]` lines

.. warning::
    Use reference peripherals. Different specs may require code/config changes.

## 2. Directory Structure

```text
h264_decode_example/
├── ap/
│   ├── ap_main.c                                  # AP entry, registers the h264_decode CLI and starts the boot demo
│   ├── config/bk7259_ap/                          # AP-side defconfig / GPIO config
│   └── h264_decode/
│       ├── common/
│       │   ├── h264_decode_h264_parser.c/.h       # Shared Annex-B AU / frame-type parser
│       │   ├── h264_decode_stream_1280x720.c/.h   # 1I30P stream (no B-frames)
│       │   └── h264_decode_stream_1280x720_ibbp.c # IBBP stream (with B-frames)
│       ├── include/
│       │   ├── h264_decode_test.h                 # CLI entry / stream enum declarations
│       │   └── vcdec_h264_test_common.h           # Shared test-helper declarations
│       └── src/
│           ├── h264_decode_cli.c                  # h264_decode CLI dispatch
│           ├── vcdec_h264_test_common.c           # Shared helpers + stream table + result line
│           ├── vcdec_h264_frame_test.c            # frame controller test (non-B)
│           ├── vcdec_h264_flexa_test.c            # flexa controller test (non-B)
│           ├── vcdec_h264_frame_zerocopy_test.c   # zero-copy / B-frame controller test
│           └── vcdec_h264_boot_demo.c             # boot-time auto-run demo
├── cp/
├── partitions/
└── .it.csv
```

## 3. Features

- Three `bk_decoder` (`bk_h264_decode_ctlr`) controllers, each test case in its own source file:
  - `h264_decode vcdec_h264d [stream]` — frame controller (whole-frame, **non-B**), `vcdec_h264_frame_test.c`
  - `h264_decode vcdec_h264d_flexa [stream]` — flexa/segmented controller (**non-B**), `vcdec_h264_flexa_test.c`
  - `h264_decode vcdec_h264d_frame_zerocopy [stream]` — zero-copy / **B-frame** controller, `vcdec_h264_frame_zerocopy_test.c`
- Two 1280x720 streams are embedded simultaneously (each under its own symbol):
  - `1280x720_1i30p` — baseline 1 IDR + 30 P frames, no B-frames (symbol `h264_decode_stream_1280x720_1i30p[]`)
  - `1280x720_ibbp` — Main profile with B-frames (symbol `h264_decode_stream_1280x720_ibbp[]`; `1280x720` is an alias for it)
- Controller / stream compatibility: the frame and flexa controllers are non-B and only work
  with `1280x720_1i30p`; the frame-zerocopy controller supports both streams and is the only
  one that decodes B-frames.
- Shared H264 parser for Annex-B access units, frame-type detection, and frame-table
  generation; reused by all three test files via `vcdec_h264_test_common.c`.
- Boot self-test demo (`vcdec_h264_run_boot_demo()` in `vcdec_h264_boot_demo.c`): runs
  frame(1i30p), flexa(1i30p) and frame-zerocopy(ibbp) once each in a dedicated thread.

## 4. Build And Run

### 4.1 Build

```bash
make bk7259 PROJECT=multimedia/h264_decode_example -j32
```

### 4.2 CLI Commands

```text
h264_decode help
h264_decode vcdec_h264d 1280x720_1i30p
h264_decode vcdec_h264d_flexa 1280x720_1i30p
h264_decode vcdec_h264d_frame_zerocopy 1280x720_1i30p
h264_decode vcdec_h264d_frame_zerocopy 1280x720_ibbp
```

Note: the frame / flexa controllers are non-B and must use `1280x720_1i30p`; the
frame-zerocopy controller accepts both `1280x720_1i30p` and `1280x720_ibbp`. When no stream
argument is given, the default is `1280x720_ibbp` (`1280x720` is an alias for it).

Success / failure of command submission: `CMDRSP:OK` / `CMDRSP:ERROR`.

### 4.3 How To Judge Pass Or Fail

`CMDRSP:OK` only means the CLI created the worker thread successfully; it does not mean the
test passed. At the end of a run, check the per-controller result line:

```text
[RESULT][PASS] vcdec_h264_test success, decoded_aus=..., rounds=...                 # vcdec_h264d (frame)
[RESULT][PASS] vcdec_h264_flexa_test success, decoded_aus=..., rounds=...           # vcdec_h264d_flexa
[RESULT][PASS] vcdec_h264_frame_zerocopy_test success, decoded_aus=..., rounds=...  # vcdec_h264d_frame_zerocopy
```

On failure the matching `[RESULT][FAIL] <case> failed at <stage>, ret=...` line is printed.

## 5. Notes

1. The `h264_decode` CLI provides `vcdec_h264d`, `vcdec_h264d_flexa`, and
   `vcdec_h264d_frame_zerocopy`, each optionally taking a stream argument (`1280x720_1i30p`
   or `1280x720_ibbp`).
2. Controller / stream compatibility: the frame and flexa controllers are non-B and only
   decode `1280x720_1i30p`; the frame-zerocopy controller decodes both streams (it is the
   only one that handles the B-frame `1280x720_ibbp` stream).
3. All test cases and the boot demo run in dedicated worker threads to avoid blocking the CLI
   thread; the boot demo and manual CLI share the decoder hardware, so wait for the boot demo
   to finish before triggering tests manually.
4. If no stream argument is given, the CLI defaults to `1280x720_ibbp`; `1280x720` is an alias
   for it.
5. The default `bk7259_ap` configuration enables `CONFIG_BK_DECODER=y` and
   `CONFIG_FRAME_BUFFER=y`.
6. Both 1280x720 streams are embedded into flash simultaneously, each under its own symbol
   (`h264_decode_stream_1280x720_1i30p[]` and `h264_decode_stream_1280x720_ibbp[]`); the
   stream is chosen at run time by the stream id / CLI argument, and both fit comfortably.
7. The old `vcdec_h264_driver` (register-level), `h264_decode_flexa`, `h264_decode_stress`
   CLIs and the `256x128` stream are no longer part of this project; the legacy sources are
   archived under `ap/properties/modules/verisilicon_nano/legacy/projects/h264_decode_example/`.
