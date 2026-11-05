/*
 * sdload - background PSRAM/CPU/memory-bus load generator.
 *
 * Runs a background task that continuously moves a configurable amount of
 * data through the AP PSRAM heap, to emulate the CPU and bus activity of a
 * real workload while the SDIO/FATFS stack is under test.
 *
 * The module is a library only: the "sdcard" command registers the CLI and
 * drives sdload_start() / sdload_stop() / sdload_get_info().
 *
 * Memory: two PSRAM buffers of work_kb each. The AP_PSRAM_HEAP in this
 * project is 512KB and the SD driver also keeps a 32KB RX bounce buffer in
 * it, so a single buffer is capped at 192KB.
 */
#ifndef __SDLOAD_H__
#define __SDLOAD_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SDLOAD_OK             0
#define SDLOAD_ERR_PARAM     -1   /* work_kb/mode/chunk out of range */
#define SDLOAD_ERR_NOMEM     -2   /* PSRAM allocation failed */
#define SDLOAD_ERR_THREAD    -3   /* task creation failed */
#define SDLOAD_ERR_BUSY      -4   /* already running */
#define SDLOAD_ERR_TIMEOUT   -5   /* task did not exit in time */

#define SDLOAD_MODE_COPY      1   /* memcpy src -> dst (read + write traffic) */
#define SDLOAD_MODE_FILL      2   /* memset dst (write traffic) */
#define SDLOAD_MODE_READ      3   /* checksum read of src (read traffic) */

#define SDLOAD_MAX_WORK_KB     192u
#define SDLOAD_DEFAULT_WORK_KB 128u
#define SDLOAD_DEFAULT_CHUNK_KB 64u

typedef struct {
	uint32_t mode;        /* SDLOAD_MODE_* */
	uint32_t work_kb;     /* per-buffer size */
	uint32_t chunk_kb;    /* bytes moved per inner iteration */
	uint32_t iterations;  /* completed buffer passes */
	uint32_t elapsed_ms;  /* since start */
} sdload_info_t;

/* Start the load task. work_kb: 1..SDLOAD_MAX_WORK_KB, mode: SDLOAD_MODE_*,
 * chunk_kb: 0 selects the default and is clamped to work_kb. */
int sdload_start(uint32_t work_kb, uint32_t mode, uint32_t chunk_kb);

/* Stop the task and wait for its buffers to be released. */
int sdload_stop(void);

/* Non-zero while the load task is running. */
int sdload_is_running(void);

/* Fill *info with the current parameters and counters. Returns 0 on success. */
int sdload_get_info(sdload_info_t *info);

#ifdef __cplusplus
}
#endif

#endif /* __SDLOAD_H__ */
