# Frame Buffer Example

* [中文](./README_CN.md)

## 1. Overview

`frame_buffer_example` is a frame-buffer memory-management example for BK7259. It demonstrates AVDK frame-buffer initialization, buffer allocation, data writes, memory dump, buffer release and heap-state inspection. It is useful for learning how multimedia buffers are allocated from slab heaps and how to debug buffer memory layout.

This example includes:

- Initialization with `bk_frame_buffer_init()`.
- Basic usage of `bk_frame_buffer_malloc()` and `bk_frame_buffer_free()`.
- Allocation from `MEM_SLAB_HEAP_UNCODED`.
- Heap-state output with `bk_mem_slab_dump_heap()`.
- Memory dump output with `avdk_hex_dump()`.
- Optional frame-buffer overflow detection cases.

## 2. Hardware Requirements

- SoC/board: BK7259 series development board.
- Debug interface: UART console for logs.

This example does not require an LCD, camera, USB device or SD card.

## 3. Project Structure

```text
frame_buffer_example/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── ap_main.c             # AP entry, starts the frame-buffer test
│   ├── frame_buffer_test.c   # Allocation, free and overflow detection cases
│   ├── frame_buffer_test.h
│   └── config/
├── cp/
│   ├── cp_main.c
│   └── config/
└── partitions/
```

## 4. Build And Flash

Run the build command from the SDK root directory:

```bash
make bk7259 PROJECT=multimedia/frame_buffer_example -j
```

After the build completes, the firmware image is generated at:

```text
build/bk7259/frame_buffer_example/package/all-app.bin
```

Flash `all-app.bin` to the board. After flashing, reset the board and open the UART terminal to check logs.

## 5. Runtime Flow

The default test runs automatically after boot. No CLI command is required. The default flow is:

1. Initialize the system with `bk_init()`.
2. Initialize the frame-buffer module in `frame_buffer_test()`.
3. Allocate three 64-byte buffers in `frame_buffer_mem_dump_test()`.
4. Fill the buffers with `0x11`, `0x22` and `0x33`.
5. Print heap status and hexadecimal memory content around each buffer.
6. Free the three buffers.
7. Print heap status and memory content again.

UART logs should contain output similar to:

```text
M55 main running...
frame_buffer_mem_dump_test: start
frame_buffer_mem_dump_test: frame1: 0x...
frame_buffer_mem_dump_test: frame2: 0x...
frame_buffer_mem_dump_test: frame3: 0x...
frame_buffer_mem_dump_test: end
```

The log also contains heap information from `bk_mem_slab_dump_heap()` and memory data from `avdk_hex_dump()`.

## 6. Overflow Detection Cases

`ap/frame_buffer_test.c` keeps two overflow-write detection cases:

- `frame_buffer_mem_overflow_test1()`: writes beyond the target buffer, then frees the buffer so the free flow can detect the corruption.
- `frame_buffer_mem_overflow_test2()`: writes beyond the target buffer, then calls `bk_mem_slab_check_all_heaps()` to check all heaps.

Both cases are disabled by default:

```c
//frame_buffer_mem_overflow_test1();
//frame_buffer_mem_overflow_test2();
```

To verify overflow detection, uncomment one function call in `frame_buffer_test()`, then rebuild and flash the firmware. Enable only one overflow case at a time.

Note: the overflow cases intentionally corrupt the buffer guard region. Error logs, asserts or system exceptions are expected and do not indicate that the default frame-buffer allocation flow is broken.

## 7. Key Configuration

This example depends on the following main configuration options:

```text
CONFIG_FRAME_BUFFER=y
CONFIG_SYS_PRINT_DEV_MAILBOX=y
```

Network and unrelated peripheral options are disabled by default to reduce dependencies on other modules.

## 8. Notes

1. The default test only demonstrates allocation and release. It does not keep any background task running.
2. The `avdk_hex_dump()` range includes guard regions before and after the user buffer so the memory layout can be inspected.
3. Overflow detection cases require source modification and are intended for debugging or validating memory protection behavior.
4. If other multimedia modules are added to the project, available frame-buffer heap space may change.

## 9. Troubleshooting

### The Log Shows `malloc failed`

Check whether the frame-buffer heap has enough free space. If other modules were added to the project, also check whether they have already consumed slab heap memory.

### No Hex Dump Is Printed

Check the UART output configuration and log level. The example mainly uses `BK_LOGI` and system logs are routed through mailbox output.

### The System Fails After Enabling An Overflow Case

The overflow cases intentionally corrupt guard regions. Asserts, exceptions or error logs are expected. Comment out the overflow case again to restore the default test.
