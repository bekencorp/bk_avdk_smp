// Copyright 2020-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cli.h"
#include "diskio.h"
#include "ff.h"
#include "sdload/sdload.h"

#include <driver/gpio.h>
#include <driver/sd_card.h>
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <stdbool.h>

extern void bk_task_wdt_feed(void);

#define SDTEST_DRIVE                    DISK_NUMBER_SDIO_SD
#define SDTEST_VOLUME                   "1:"
#define SDTEST_DIR                      "1:/"
#define SDTEST_BENCH_FILE               SDTEST_DIR "sdtest_bench.bin"
#define SDTEST_RW_FILE                  SDTEST_DIR "sdtest_rw.bin"
#define SDTEST_RAND_FILE                SDTEST_DIR "sdtest_rand.bin"
#define SDTEST_SOAK_FILE                SDTEST_DIR "sdtest_soak.bin"

#define SDTEST_IO_CHUNK                 (64u * 1024u)
#define SDTEST_RW_DEFAULT_KB            1024u
#define SDTEST_MAX_KB                   (64u * 1024u)
#define SDTEST_BENCH_DEFAULT_MB         16u
#define SDTEST_BENCH_DEFAULT_ROUNDS     3u
#define SDTEST_BENCH_MAX_MB             64u
#define SDTEST_BENCH_MAX_ROUNDS         10u
#define SDTEST_RAND_DEFAULT_CHUNK_KB    4u
#define SDTEST_RAND_MAX_CHUNK_KB        64u
#define SDTEST_RAND_MAX_ROUNDS          100u
#define SDTEST_SOAK_DEFAULT_KB          1024u
#define SDTEST_SOAK_DEFAULT_SEC         60u
#define SDTEST_SOAK_MAX_SEC             (24u * 60u * 60u)
#define SDTEST_MOUNTTEST_DEFAULT_ROUNDS 10u
#define SDTEST_MOUNTTEST_MAX_ROUNDS     1000u
#define SDTEST_MOUNTTEST_DEFAULT_DELAY  100u
#define SDTEST_MOUNTTEST_MAX_DELAY      10000u
#define SDTEST_STOP_POLL_MS             10u
#define SDTEST_STOP_POLL_CNT            500u
#define SDTEST_SEED_DEFAULT             0x5au

#if CONFIG_SDCARD_BUSWIDTH_4LINE
#define SDTEST_BUS_WIDTH                4u
#else
#define SDTEST_BUS_WIDTH                1u
#endif

static FATFS *s_fs = NULL;
static bool s_mounted = false;

static beken_thread_t s_soak_thread = NULL;
static volatile bool s_soak_run = false;
static volatile bool s_soak_stop = false;
static uint32_t s_soak_size_kb = 0;
static uint32_t s_soak_duration_s = 0;
static uint32_t s_soak_start_ms = 0;
static uint32_t s_soak_elapsed_ms = 0;
static volatile uint32_t s_soak_iterations = 0;
static volatile uint32_t s_soak_failures = 0;
static volatile uint32_t s_soak_total_kb = 0;
static volatile uint32_t s_soak_write_ms = 0;
static volatile uint32_t s_soak_read_ms = 0;

typedef struct {
	uint32_t rounds;
	uint32_t total_bytes;
	uint32_t write_rate_sum;
	uint32_t read_rate_sum;
} sdtest_bench_result_t;

static const char *sdtest_fr_name(FRESULT fr)
{
	switch (fr) {
	case FR_OK: return "OK";
	case FR_DISK_ERR: return "DISK_ERR";
	case FR_INT_ERR: return "INT_ERR";
	case FR_NOT_READY: return "NOT_READY";
	case FR_NO_FILE: return "NO_FILE";
	case FR_NO_PATH: return "NO_PATH";
	case FR_INVALID_NAME: return "INVALID_NAME";
	case FR_DENIED: return "DENIED";
	case FR_EXIST: return "EXIST";
	case FR_INVALID_OBJECT: return "INVALID_OBJECT";
	case FR_WRITE_PROTECTED: return "WRITE_PROTECTED";
	case FR_INVALID_DRIVE: return "INVALID_DRIVE";
	case FR_NOT_ENABLED: return "NOT_ENABLED";
	case FR_NO_FILESYSTEM: return "NO_FILESYSTEM";
	case FR_MKFS_ABORTED: return "MKFS_ABORTED";
	case FR_TIMEOUT: return "TIMEOUT";
	case FR_LOCKED: return "LOCKED";
	case FR_NOT_ENOUGH_CORE: return "NOT_ENOUGH_CORE";
	case FR_TOO_MANY_OPEN_FILES: return "TOO_MANY_OPEN_FILES";
	case FR_INVALID_PARAMETER: return "INVALID_PARAMETER";
	default: return "UNKNOWN";
	}
}

static uint32_t sdtest_rate_x100(uint32_t bytes, uint32_t ms)
{
	uint64_t scaled;

	if (ms == 0)
		ms = 1;

	scaled = (uint64_t)bytes * 1000u * 100u;
	return (uint32_t)(scaled / ((uint64_t)ms * 1024u * 1024u));
}

static void sdtest_print_rate(const char *op, const char *path, uint32_t bytes,
			      uint32_t ms, FRESULT fr)
{
	uint32_t rate = sdtest_rate_x100(bytes, ms);

	os_printf("[SDTEST] %s file=%s bytes=%u ms=%u mb_s=%u.%02u fr=%d(%s)\r\n",
		  op, path, (unsigned)bytes, (unsigned)ms,
		  (unsigned)(rate / 100u), (unsigned)(rate % 100u),
		  fr, sdtest_fr_name(fr));
}

static bool sdtest_load_on(void)
{
	return sdload_is_running() ? true : false;
}

static const char *sdtest_load_mode_name(uint32_t mode)
{
	switch (mode) {
	case SDLOAD_MODE_FILL: return "fill";
	case SDLOAD_MODE_READ: return "read";
	case SDLOAD_MODE_COPY: return "copy";
	default: return "unknown";
	}
}

static int sdtest_fs_busy(void)
{
	if (!s_soak_run && (s_soak_thread == NULL))
		return 0;

	os_printf("[SDTEST] soak is running, stop it first: sdcard soak stop\r\n");
	return 1;
}

static void sdtest_make_path(const char *name, char *path, uint32_t size)
{
	if (os_strchr(name, ':') != NULL)
		os_snprintf(path, size, "%s", name);
	else
		os_snprintf(path, size, SDTEST_DIR "%s", name);
}

static uint8_t sdtest_pattern(uint32_t offset, uint32_t seed)
{
	return (uint8_t)(seed ^ (offset & 0xffu) ^
			 ((offset >> 8) & 0xffu) ^
			 ((offset >> 16) & 0xffu));
}

static void sdtest_fill_pattern(uint8_t *buf, uint32_t len, uint32_t offset,
				uint32_t seed)
{
	uint32_t i;

	for (i = 0; i < len; i++)
		buf[i] = sdtest_pattern(offset + i, seed);
}

static uint32_t sdtest_check_pattern(const uint8_t *buf, uint32_t len,
				     uint32_t offset, uint32_t seed)
{
	uint32_t i;
	uint32_t bad = 0;
	uint32_t first = 0;

	for (i = 0; i < len; i++) {
		if (buf[i] != sdtest_pattern(offset + i, seed)) {
			if (bad == 0)
				first = offset + i;
			bad++;
		}
	}

	if (bad != 0)
		os_printf("[SDTEST] MISMATCH first_offset=%u count=%u\r\n",
			  (unsigned)first, (unsigned)bad);

	return bad;
}

static FRESULT sdtest_mount(void)
{
	FRESULT fr;

	if (s_mounted)
		return FR_OK;

	s_fs = (FATFS *)os_malloc(sizeof(FATFS));
	if (s_fs == NULL) {
		os_printf("[SDTEST] mount: alloc FATFS failed\r\n");
		return FR_NOT_ENOUGH_CORE;
	}

	fr = f_mount(s_fs, SDTEST_VOLUME, 1);
	if (fr != FR_OK) {
		os_printf("[SDTEST] mount %s failed fr=%d(%s)\r\n",
			  SDTEST_VOLUME, fr, sdtest_fr_name(fr));
		(void)f_mount(NULL, SDTEST_VOLUME, 0);
		os_free(s_fs);
		s_fs = NULL;
		return fr;
	}

	s_mounted = true;
	os_printf("[SDTEST] mount %s OK\r\n", SDTEST_VOLUME);
	return FR_OK;
}

static FRESULT sdtest_unmount(void)
{
	FRESULT fr;

	if (!s_mounted)
		return FR_OK;

	fr = f_unmount((BYTE)SDTEST_DRIVE, SDTEST_VOLUME, 1);
	if (fr != FR_OK) {
		os_printf("[SDTEST] umount failed fr=%d(%s), force detach\r\n",
			  fr, sdtest_fr_name(fr));
		(void)f_mount(NULL, SDTEST_VOLUME, 0);
	} else {
		os_printf("[SDTEST] umount %s OK\r\n", SDTEST_VOLUME);
	}

	os_free(s_fs);
	s_fs = NULL;
	s_mounted = false;
	return fr;
}

static int sdtest_require_mount(void)
{
	return (sdtest_mount() == FR_OK) ? 0 : -1;
}

static FRESULT sdtest_write_data(const char *path, uint32_t bytes,
				 uint32_t seed, bool sync_each_chunk,
				 bool fill_pattern, uint8_t *buf,
				 volatile bool *stop,
				 uint32_t *elapsed_ms)
{
	FIL file;
	FRESULT fr;
	UINT written = 0;
	uint32_t offset = 0;
	uint32_t start_ms;

	fr = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		os_printf("[SDTEST] write open %s failed fr=%d(%s)\r\n",
			  path, fr, sdtest_fr_name(fr));
		return fr;
	}

	start_ms = rtos_get_time();
	while (offset < bytes) {
		uint32_t chunk = bytes - offset;

		if ((stop != NULL) && *stop)
			break;

		if (chunk > SDTEST_IO_CHUNK)
			chunk = SDTEST_IO_CHUNK;
		if (fill_pattern)
			sdtest_fill_pattern(buf, chunk, offset, seed);

		fr = f_write(&file, buf, chunk, &written);
		if (written != chunk) {
			if (fr == FR_OK)
				fr = FR_DENIED;
			os_printf("[SDTEST] write %s failed fr=%d(%s) "
				  "written=%u/%u\r\n", path, fr,
				  sdtest_fr_name(fr), (unsigned)written,
				  (unsigned)chunk);
			break;
		}
		if (fr != FR_OK) {
			os_printf("[SDTEST] write %s failed fr=%d(%s)\r\n",
				  path, fr, sdtest_fr_name(fr));
			break;
		}

		offset += written;
		if (sync_each_chunk) {
			fr = f_sync(&file);
			if (fr != FR_OK) {
				os_printf("[SDTEST] write f_sync failed "
					  "fr=%d(%s)\r\n", fr,
					  sdtest_fr_name(fr));
				break;
			}
		}
		bk_task_wdt_feed();
	}

	if (fr == FR_OK) {
		fr = f_sync(&file);
		if (fr != FR_OK)
			os_printf("[SDTEST] write final f_sync failed fr=%d(%s)\r\n",
				  fr, sdtest_fr_name(fr));
	}

	{
		FRESULT close_fr = f_close(&file);

		if (fr == FR_OK)
			fr = close_fr;
	}

	*elapsed_ms = rtos_get_time() - start_ms;
	return fr;
}

static FRESULT sdtest_read_data(const char *path, uint32_t bytes, bool verify,
				uint32_t seed, uint8_t *buf,
				volatile bool *stop,
				uint32_t *elapsed_ms, uint32_t *mismatches,
				uint32_t *actual_bytes)
{
	FIL file;
	FRESULT fr;
	UINT got = 0;
	uint32_t offset = 0;
	uint32_t start_ms;
	uint32_t bad = 0;

	if (mismatches != NULL)
		*mismatches = 0;
	if (actual_bytes != NULL)
		*actual_bytes = 0;

	fr = f_open(&file, path, FA_OPEN_EXISTING | FA_READ);
	if (fr != FR_OK) {
		os_printf("[SDTEST] read open %s failed fr=%d(%s)\r\n",
			  path, fr, sdtest_fr_name(fr));
		return fr;
	}

	if (bytes == 0)
		bytes = (uint32_t)f_size(&file);

	start_ms = rtos_get_time();
	while (offset < bytes) {
		uint32_t chunk = bytes - offset;

		if ((stop != NULL) && *stop)
			break;

		if (chunk > SDTEST_IO_CHUNK)
			chunk = SDTEST_IO_CHUNK;

		fr = f_read(&file, buf, chunk, &got);
		if (fr != FR_OK) {
			os_printf("[SDTEST] read %s failed fr=%d(%s)\r\n",
				  path, fr, sdtest_fr_name(fr));
			break;
		}
		if (got == 0)
			break;

		if (verify)
			bad += sdtest_check_pattern(buf, got, offset, seed);

		offset += got;
		if (got != chunk)
			break;

		bk_task_wdt_feed();
	}

	(void)f_close(&file);
	*elapsed_ms = rtos_get_time() - start_ms;
	if (mismatches != NULL)
		*mismatches = bad;
	if (actual_bytes != NULL)
		*actual_bytes = offset;
	return fr;
}

static uint32_t sdtest_default_capacity(void)
{
#if (CONFIG_SDCARD_HOST_ID == 0)
	return GPIO_DRIVER_CAPACITY_3;
#else
	return GPIO_DRIVER_CAPACITY_1;
#endif
}

static void sdtest_apply_drive_capacity(uint32_t capacity)
{
#if (CONFIG_SDCARD_HOST_ID == 0)
	gpio_id_t pin;

	for (pin = GPIO_2; pin <= GPIO_4; pin++)
		(void)bk_gpio_set_capacity(pin, capacity);
#else
	gpio_id_t pin;

	for (pin = GPIO_14; pin <= GPIO_19; pin++)
		(void)bk_gpio_set_capacity(pin, capacity);
#endif
}

static int sdtest_bench_run(uint32_t size_mb, uint32_t rounds,
			    uint32_t capacity, bool apply_capacity,
			    sdtest_bench_result_t *result)
{
	uint8_t *buf;
	uint32_t total_bytes;
	uint32_t total = 0;
	uint32_t write_rate_sum = 0;
	uint32_t read_rate_sum = 0;
	uint32_t round;
	int ret = -1;

	buf = (uint8_t *)os_malloc(SDTEST_IO_CHUNK);
	if (buf == NULL) {
		os_printf("[SDTEST] bench: alloc %u bytes failed\r\n",
			  (unsigned)SDTEST_IO_CHUNK);
		return -1;
	}

	for (total = 0; total < SDTEST_IO_CHUNK; total++)
		buf[total] = (uint8_t)(total ^ (total >> 8));

	total_bytes = size_mb * 1024u * 1024u;
	if (apply_capacity)
		sdtest_apply_drive_capacity(capacity);

	os_printf("[SDTEST] BENCH START size_mb=%u rounds=%u chunk_kb=%u "
		  "capacity=%u clock_hz=%u bus_width=%u high_speed=%u load=%s\r\n",
		  (unsigned)size_mb, (unsigned)rounds,
		  (unsigned)(SDTEST_IO_CHUNK / 1024u), (unsigned)capacity,
		  (unsigned)CONFIG_SDCARD_CLOCK_FREQ_HZ, SDTEST_BUS_WIDTH,
#if CONFIG_SDCARD_HIGH_SPEED
		  1u,
#else
		  0u,
#endif
		  sdtest_load_on() ? "on" : "off");

	for (round = 1; round <= rounds; round++) {
		uint32_t write_ms = 0;
		uint32_t read_ms = 0;
		uint32_t write_rate;
		uint32_t read_rate;
		FRESULT fr;

		fr = sdtest_write_data(SDTEST_BENCH_FILE, total_bytes, 0, false,
				       false, buf, NULL, &write_ms);
		if (fr != FR_OK)
			goto out;

		fr = sdtest_read_data(SDTEST_BENCH_FILE, total_bytes, false, 0,
				      buf, NULL, &read_ms, NULL, NULL);
		if (fr != FR_OK)
			goto out;

		write_rate = sdtest_rate_x100(total_bytes, write_ms);
		read_rate = sdtest_rate_x100(total_bytes, read_ms);
		write_rate_sum += write_rate;
		read_rate_sum += read_rate;

		os_printf("[SDTEST] BENCH RESULT round=%u write_ms=%u "
			  "write_mb_s=%u.%02u read_ms=%u read_mb_s=%u.%02u\r\n",
			  (unsigned)round, (unsigned)write_ms,
			  (unsigned)(write_rate / 100u),
			  (unsigned)(write_rate % 100u), (unsigned)read_ms,
			  (unsigned)(read_rate / 100u),
			  (unsigned)(read_rate % 100u));
	}

	os_printf("[SDTEST] BENCH SUMMARY rounds=%u avg_write_mb_s=%u.%02u "
		  "avg_read_mb_s=%u.%02u load=%s\r\n",
		  (unsigned)rounds,
		  (unsigned)((write_rate_sum / rounds) / 100u),
		  (unsigned)((write_rate_sum / rounds) % 100u),
		  (unsigned)((read_rate_sum / rounds) / 100u),
		  (unsigned)((read_rate_sum / rounds) % 100u),
		  sdtest_load_on() ? "on" : "off");

	if (result != NULL) {
		result->rounds = rounds;
		result->total_bytes = total_bytes;
		result->write_rate_sum = write_rate_sum;
		result->read_rate_sum = read_rate_sum;
	}

	ret = 0;

out:
	(void)f_unlink(SDTEST_BENCH_FILE);
	os_free(buf);
	return ret;
}

static int sdtest_rwtest_run(uint32_t size_kb, uint32_t rounds, bool print_each,
			     uint32_t *pass_rounds)
{
	uint8_t *buf;
	uint32_t bytes = size_kb * 1024u;
	uint32_t round;
	uint32_t failures = 0;

	buf = (uint8_t *)os_malloc(SDTEST_IO_CHUNK);
	if (buf == NULL) {
		os_printf("[SDTEST] rwtest: alloc %u bytes failed\r\n",
			  (unsigned)SDTEST_IO_CHUNK);
		return -1;
	}

	for (round = 1; round <= rounds; round++) {
		uint32_t seed = round;
		uint32_t write_ms = 0;
		uint32_t read_ms = 0;
		uint32_t mismatches = 0;
		uint32_t got = 0;
		FRESULT fr;

		fr = sdtest_write_data(SDTEST_RW_FILE, bytes, seed, false, true,
				       buf, NULL, &write_ms);
		if (fr == FR_OK)
			fr = sdtest_read_data(SDTEST_RW_FILE, bytes, true, seed,
					      buf, NULL, &read_ms, &mismatches,
					      &got);

		if ((fr != FR_OK) || (got != bytes) || (mismatches != 0)) {
			failures++;
			os_printf("[SDTEST] RWTEST round=%u FAIL fr=%d(%s) "
				  "got=%u/%u mismatches=%u load=%s\r\n",
				  (unsigned)round, fr, sdtest_fr_name(fr),
				  (unsigned)got, (unsigned)bytes,
				  (unsigned)mismatches,
				  sdtest_load_on() ? "on" : "off");
		} else if (print_each) {
			uint32_t write_rate = sdtest_rate_x100(bytes, write_ms);
			uint32_t read_rate = sdtest_rate_x100(bytes, read_ms);

			os_printf("[SDTEST] RWTEST round=%u PASS write_mb_s=%u.%02u "
				  "read_mb_s=%u.%02u load=%s\r\n",
				  (unsigned)round,
				  (unsigned)(write_rate / 100u),
				  (unsigned)(write_rate % 100u),
				  (unsigned)(read_rate / 100u),
				  (unsigned)(read_rate % 100u),
				  sdtest_load_on() ? "on" : "off");
		}

		bk_task_wdt_feed();
	}

	(void)f_unlink(SDTEST_RW_FILE);
	os_free(buf);

	if (pass_rounds != NULL)
		*pass_rounds = rounds - failures;

	return failures ? -1 : 0;
}

static uint32_t sdtest_rand_next(uint32_t *state, uint32_t max_block)
{
	*state = (*state * 1103515245u) + 12345u;
	return ((*state >> 8) % max_block) * 512u;
}

static int sdtest_randrw_run(uint32_t size_kb, uint32_t chunk_kb,
			     uint32_t rounds)
{
	uint8_t *buf;
	uint32_t bytes = size_kb * 1024u;
	uint32_t chunk = chunk_kb * 1024u;
	uint32_t chunks;
	uint32_t max_block;
	uint32_t round;
	uint32_t failures = 0;
	FIL file;
	FRESULT fr;

	if (chunk > bytes)
		chunk = bytes;
	chunks = bytes / chunk;
	if (chunks == 0) {
		os_printf("[SDTEST] randrw: invalid chunk\r\n");
		return -1;
	}
	max_block = ((bytes - chunk) / 512u) + 1u;

	buf = (uint8_t *)os_malloc(chunk);
	if (buf == NULL) {
		os_printf("[SDTEST] randrw: alloc %u bytes failed\r\n",
			  (unsigned)chunk);
		return -1;
	}

	/* Preallocate the file so every random write lands inside it. */
	fr = f_open(&file, SDTEST_RAND_FILE, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr == FR_OK) {
		fr = f_lseek(&file, bytes - 1);
		if (fr == FR_OK) {
			uint8_t zero = 0;
			UINT written = 0;

			fr = f_write(&file, &zero, 1, &written);
			if ((fr == FR_OK) && (written != 1))
				fr = FR_DENIED;
		}
		{
			FRESULT close_fr = f_close(&file);

			if (fr == FR_OK)
				fr = close_fr;
		}
	}
	if (fr != FR_OK) {
		os_printf("[SDTEST] randrw preallocate failed fr=%d(%s)\r\n",
			  fr, sdtest_fr_name(fr));
		(void)f_unlink(SDTEST_RAND_FILE);
		os_free(buf);
		return -1;
	}

	os_printf("[SDTEST] RANDRW START size_kb=%u chunk_kb=%u rounds=%u "
		  "chunks=%u load=%s\r\n", (unsigned)size_kb,
		  (unsigned)chunk_kb, (unsigned)rounds, (unsigned)chunks,
		  sdtest_load_on() ? "on" : "off");

	for (round = 1; round <= rounds; round++) {
		uint32_t seed = round;
		uint32_t state;
		uint32_t write_ms = 0;
		uint32_t read_ms = 0;
		uint32_t bad = 0;
		uint32_t i;
		uint32_t start_ms;

		/* Write phase. */
		fr = f_open(&file, SDTEST_RAND_FILE,
			    FA_OPEN_EXISTING | FA_WRITE);
		if (fr != FR_OK) {
			os_printf("[SDTEST] randrw write open failed fr=%d(%s)\r\n",
				  fr, sdtest_fr_name(fr));
			failures++;
			continue;
		}

		state = 0x12345678u + round;
		start_ms = rtos_get_time();
		for (i = 0; i < chunks; i++) {
			uint32_t offset = sdtest_rand_next(&state, max_block);
			UINT written = 0;

			fr = f_lseek(&file, offset);
			if (fr != FR_OK)
				break;
			sdtest_fill_pattern(buf, chunk, offset, seed);
			fr = f_write(&file, buf, chunk, &written);
			if ((fr == FR_OK) && (written != chunk))
				fr = FR_DENIED;
			if (fr != FR_OK)
				break;
			bk_task_wdt_feed();
		}
		if (fr == FR_OK)
			fr = f_sync(&file);
		{
			FRESULT close_fr = f_close(&file);

			if (fr == FR_OK)
				fr = close_fr;
		}
		write_ms = rtos_get_time() - start_ms;

		if (fr != FR_OK) {
			failures++;
			os_printf("[SDTEST] RANDRW round=%u write FAIL "
				  "fr=%d(%s) at=%u/%u\r\n", (unsigned)round, fr,
				  sdtest_fr_name(fr), (unsigned)i,
				  (unsigned)chunks);
			continue;
		}

		/* Read-back and verify phase, replaying the same offsets. */
		fr = f_open(&file, SDTEST_RAND_FILE,
			    FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK) {
			failures++;
			os_printf("[SDTEST] randrw read open failed fr=%d(%s)\r\n",
				  fr, sdtest_fr_name(fr));
			continue;
		}

		state = 0x12345678u + round;
		start_ms = rtos_get_time();
		for (i = 0; i < chunks; i++) {
			uint32_t offset = sdtest_rand_next(&state, max_block);
			UINT got = 0;

			fr = f_lseek(&file, offset);
			if (fr != FR_OK)
				break;
			fr = f_read(&file, buf, chunk, &got);
			if (fr != FR_OK)
				break;
			if (got != chunk) {
				fr = FR_DENIED;
				break;
			}
			bad += sdtest_check_pattern(buf, got, offset, seed);
			bk_task_wdt_feed();
		}
		(void)f_close(&file);
		read_ms = rtos_get_time() - start_ms;

		if ((fr != FR_OK) || (bad != 0)) {
			failures++;
			os_printf("[SDTEST] RANDRW round=%u FAIL fr=%d(%s) "
				  "mismatches=%u\r\n", (unsigned)round, fr,
				  sdtest_fr_name(fr), (unsigned)bad);
		} else {
			uint32_t write_rate = sdtest_rate_x100(bytes, write_ms);
			uint32_t read_rate = sdtest_rate_x100(bytes, read_ms);
			uint32_t write_iops = (uint32_t)(((uint64_t)chunks *
					1000u) / (write_ms ? write_ms : 1u));
			uint32_t read_iops = (uint32_t)(((uint64_t)chunks *
					1000u) / (read_ms ? read_ms : 1u));

			os_printf("[SDTEST] RANDRW round=%u PASS "
				  "write_mb_s=%u.%02u write_iops=%u "
				  "read_mb_s=%u.%02u read_iops=%u load=%s\r\n",
				  (unsigned)round,
				  (unsigned)(write_rate / 100u),
				  (unsigned)(write_rate % 100u),
				  (unsigned)write_iops,
				  (unsigned)(read_rate / 100u),
				  (unsigned)(read_rate % 100u),
				  (unsigned)read_iops,
				  sdtest_load_on() ? "on" : "off");
		}
	}

	(void)f_unlink(SDTEST_RAND_FILE);
	os_free(buf);

	os_printf("[SDTEST] RANDRW SUMMARY rounds=%u pass=%u fail=%u\r\n",
		  (unsigned)rounds, (unsigned)(rounds - failures),
		  (unsigned)failures);

	return failures ? -1 : 0;
}

static void sdtest_soak_report(const char *tag)
{
	uint32_t iterations = s_soak_iterations;
	uint32_t failures = s_soak_failures;
	uint32_t total_kb = s_soak_total_kb;
	uint32_t write_ms = s_soak_write_ms;
	uint32_t read_ms = s_soak_read_ms;
	uint32_t write_rate = 0;
	uint32_t read_rate = 0;

	if (write_ms != 0)
		write_rate = (uint32_t)(((uint64_t)total_kb * 100000u) /
					((uint64_t)write_ms * 1024u));
	if (read_ms != 0)
		read_rate = (uint32_t)(((uint64_t)total_kb * 100000u) /
				       ((uint64_t)read_ms * 1024u));

	os_printf("[SDTEST] %s iter=%u fail=%u total_mb=%u "
		  "avg_write_mb_s=%u.%02u avg_read_mb_s=%u.%02u load=%s\r\n",
		  tag, (unsigned)iterations, (unsigned)failures,
		  (unsigned)(total_kb / 1024u),
		  (unsigned)(write_rate / 100u),
		  (unsigned)(write_rate % 100u),
		  (unsigned)(read_rate / 100u),
		  (unsigned)(read_rate % 100u),
		  sdtest_load_on() ? "on" : "off");
}

static void sdtest_soak_task(beken_thread_arg_t arg)
{
	uint8_t *buf = (uint8_t *)os_malloc(SDTEST_IO_CHUNK);
	uint32_t start_ms = s_soak_start_ms;
	uint32_t iteration = 0;
	uint32_t bytes = s_soak_size_kb * 1024u;

	(void)arg;

	if (buf == NULL) {
		os_printf("[SDTEST] SOAK alloc %u bytes failed\r\n",
			  (unsigned)SDTEST_IO_CHUNK);
		goto out;
	}

	while (!s_soak_stop) {
		uint32_t seed;
		uint32_t write_ms = 0;
		uint32_t read_ms = 0;
		uint32_t mismatches = 0;
		uint32_t got = 0;
		FRESULT fr;

		if (s_soak_duration_s != 0) {
			uint32_t elapsed = rtos_get_time() - start_ms;

			if (elapsed >= (s_soak_duration_s * 1000u))
				break;
		}

		iteration++;
		seed = iteration;
		fr = sdtest_write_data(SDTEST_SOAK_FILE, bytes, seed, false, true,
				       buf, &s_soak_stop, &write_ms);
		if ((fr == FR_OK) && !s_soak_stop)
			fr = sdtest_read_data(SDTEST_SOAK_FILE, bytes, true, seed,
					      buf, &s_soak_stop, &read_ms,
					      &mismatches, &got);

		if (s_soak_stop)
			break;

		s_soak_iterations++;

		if ((fr != FR_OK) || (got != bytes) || (mismatches != 0)) {
			s_soak_failures++;
			os_printf("[SDTEST] SOAK FAIL iter=%u fr=%d(%s) "
				  "got=%u/%u mismatches=%u\r\n",
				  (unsigned)iteration, fr, sdtest_fr_name(fr),
				  (unsigned)got, (unsigned)bytes,
				  (unsigned)mismatches);
		} else {
			s_soak_total_kb += s_soak_size_kb;
			s_soak_write_ms += write_ms;
			s_soak_read_ms += read_ms;
		}

		if ((iteration % 10u) == 0)
			sdtest_soak_report("SOAK");

		bk_task_wdt_feed();
	}

	(void)f_unlink(SDTEST_SOAK_FILE);
	sdtest_soak_report("SOAK DONE");
	os_free(buf);

out:
	s_soak_elapsed_ms = rtos_get_time() - start_ms;
	s_soak_run = false;
	s_soak_thread = NULL;
	rtos_delete_thread(NULL);
}

static void sdtest_soak_status(void)
{
	uint32_t elapsed;

	if (s_soak_run)
		elapsed = rtos_get_time() - s_soak_start_ms;
	else
		elapsed = s_soak_elapsed_ms;

	os_printf("[SDTEST] SOAK STATUS running=%d size_kb=%u duration_s=%u "
		  "elapsed_ms=%u\r\n", s_soak_run ? 1 : 0,
		  (unsigned)s_soak_size_kb, (unsigned)s_soak_duration_s,
		  (unsigned)elapsed);
	sdtest_soak_report("SOAK STATUS");
}

static void sdtest_load_report(const char *tag)
{
	sdload_info_t info;
	uint64_t moved_bytes;
	uint32_t moved_mb;
	uint32_t rate = 0;

	if (sdload_get_info(&info) != SDLOAD_OK) {
		os_printf("[SDTEST] %s running=0\r\n", tag);
		return;
	}

	moved_bytes = (uint64_t)info.work_kb * 1024u * info.iterations;
	if (info.mode == SDLOAD_MODE_COPY)
		moved_bytes *= 2u;
	if (info.elapsed_ms != 0)
		rate = (uint32_t)((moved_bytes * 100000u) /
				  ((uint64_t)info.elapsed_ms * 1024u * 1024u));
	moved_mb = (uint32_t)(moved_bytes >> 20);

	os_printf("[SDTEST] %s running=%d mode=%s work_kb=%u chunk_kb=%u "
		  "iter=%u elapsed_ms=%u moved_mb=%u mb_s=%u.%02u\r\n",
		  tag, sdload_is_running() ? 1 : 0,
		  sdtest_load_mode_name(info.mode), (unsigned)info.work_kb,
		  (unsigned)info.chunk_kb, (unsigned)info.iterations,
		  (unsigned)info.elapsed_ms, (unsigned)moved_mb,
		  (unsigned)(rate / 100u), (unsigned)(rate % 100u));
}

static const char *sdtest_card_type_name(uint32_t type)
{
	switch (type) {
	case 0: return "SDSC";
	case 1: return "SDHC/SDXC";
	default: return "UNKNOWN";
	}
}

static const char *sdtest_card_state_name(sd_card_state_t state)
{
	switch (state) {
	case SD_CARD_IDLE: return "IDLE";
	case SD_CARD_READY: return "READY";
	case SD_CARD_IDENTIFICATION: return "IDENTIFICATION";
	case SD_CARD_STANDBY: return "STANDBY";
	case SD_CARD_TRANSFER: return "TRANSFER";
	case SD_CARD_SENDING: return "SENDING";
	case SD_CARD_RECEIVING: return "RECEIVING";
	case SD_CARD_PROGRAMMING: return "PROGRAMMING";
	case SD_CARD_DISCONNECTED: return "DISCONNECTED";
	default: return "ERROR";
	}
}

static void sdtest_help(void)
{
	os_printf("[SDTEST] commands:\r\n");
	os_printf("  sdcard mount / umount\r\n");
	os_printf("  sdcard card / info / status\r\n");
	os_printf("  sdcard write <file> <size_kb> [sync] [seed]\r\n");
	os_printf("  sdcard read <file> [size_kb] [seed]\r\n");
	os_printf("  sdcard rwtest [size_kb] [rounds]\r\n");
	os_printf("  sdcard randrw [size_kb] [chunk_kb] [rounds]\r\n");
	os_printf("  sdcard mounttest [rounds] [delay_ms]\r\n");
	os_printf("  sdcard bench [size_mb] [rounds] [drive_capacity]\r\n");
	os_printf("  sdcard soak start [size_kb] [duration_s]\r\n");
	os_printf("  sdcard soak stop / status\r\n");
	os_printf("  sdcard load start [work_kb] [mode 1=copy 2=fill 3=read] "
		  "[chunk_kb]\r\n");
	os_printf("  sdcard load stop / status\r\n");
	os_printf("  sdcard loadtest [size_mb] [rounds] [work_kb]\r\n");
	os_printf("  defaults: write/read/rwtest 1024KB, randrw 1024KB/4KB/1, "
		  "bench 16MB/3 rounds, soak 1024KB/60s, load 128KB copy\r\n");
}

static void sdtest_cmd_mount(void)
{
	uint32_t start_ms;
	FRESULT fr;

	if (sdtest_fs_busy())
		return;

	start_ms = rtos_get_time();
	fr = sdtest_mount();
	os_printf("[SDTEST] MOUNT result=%s fr=%d(%s) ms=%u\r\n",
		  (fr == FR_OK) ? "OK" : "FAIL", fr, sdtest_fr_name(fr),
		  (unsigned)(rtos_get_time() - start_ms));
}

static void sdtest_cmd_umount(void)
{
	uint32_t start_ms;
	FRESULT fr;

	if (sdtest_fs_busy())
		return;

	start_ms = rtos_get_time();
	fr = sdtest_unmount();
	os_printf("[SDTEST] UMOUNT result=%s fr=%d(%s) ms=%u\r\n",
		  (fr == FR_OK) ? "OK" : "FAIL", fr, sdtest_fr_name(fr),
		  (unsigned)(rtos_get_time() - start_ms));
}

static void sdtest_cmd_card(void)
{
	sd_card_info_t info;
	uint32_t blocks;
	uint32_t cap_gb_x100;
	bk_err_t ret;

	if (sdtest_require_mount())
		return;

	ret = bk_sd_card_get_card_info(&info);
	if (ret != BK_OK) {
		os_printf("[SDTEST] CARD get info failed ret=%d\r\n", ret);
		return;
	}

	blocks = bk_sd_card_get_card_size();
	cap_gb_x100 = blocks ?
		(uint32_t)(((uint64_t)blocks * 512u * 100u) /
			   (1024ull * 1024ull * 1024ull)) : 0;

	os_printf("[SDTEST] CARD type=%s blocks=%u capacity_gb=%u.%02u "
		  "state=%s\r\n",
		  sdtest_card_type_name(info.card_type),
		  (unsigned)blocks,
		  (unsigned)(cap_gb_x100 / 100u),
		  (unsigned)(cap_gb_x100 % 100u),
		  sdtest_card_state_name(bk_sd_card_get_card_state()));
	os_printf("[SDTEST] CARD config host=%u clock_hz=%u bus_width=%u "
		  "high_speed=%u\r\n", (unsigned)CONFIG_SDCARD_HOST_ID,
		  (unsigned)CONFIG_SDCARD_CLOCK_FREQ_HZ, SDTEST_BUS_WIDTH,
#if CONFIG_SDCARD_HIGH_SPEED
		  1u
#else
		  0u
#endif
		  );
}

static void sdtest_cmd_info(void)
{
	FATFS *fs = NULL;
	DWORD free_clusters = 0;
	uint64_t cluster_bytes;
	uint64_t total_bytes;
	uint64_t free_bytes;
	uint32_t total_mb;
	uint32_t free_mb;
	FRESULT fr;

	if (sdtest_fs_busy())
		return;
	if (sdtest_require_mount())
		return;

	fr = f_getfree(SDTEST_VOLUME, &free_clusters, &fs);
	if ((fr != FR_OK) || (fs == NULL)) {
		os_printf("[SDTEST] INFO f_getfree failed fr=%d(%s)\r\n",
			  fr, sdtest_fr_name(fr));
		return;
	}

	cluster_bytes = (uint64_t)fs->csize * fs->ssize;
	total_bytes = (uint64_t)(fs->n_fatent - 2) * cluster_bytes;
	free_bytes = (uint64_t)free_clusters * cluster_bytes;
	total_mb = (uint32_t)(total_bytes >> 20);
	free_mb = (uint32_t)(free_bytes >> 20);

	os_printf("[SDTEST] INFO total_mb=%u free_mb=%u used_mb=%u "
		  "cluster_bytes=%u load=%s\r\n", (unsigned)total_mb,
		  (unsigned)free_mb, (unsigned)(total_mb - free_mb),
		  (unsigned)cluster_bytes, sdtest_load_on() ? "on" : "off");
	bk_task_wdt_feed();
}

static void sdtest_cmd_write(int argc, char **argv)
{
	char path[64];
	uint32_t size_kb;
	uint32_t seed = SDTEST_SEED_DEFAULT;
	uint32_t elapsed_ms = 0;
	bool sync_each_chunk = false;
	uint8_t *buf;
	FRESULT fr;

	if (argc < 4) {
		os_printf("[SDTEST] usage: sdcard write <file> <size_kb> "
			  "[sync 0/1] [seed]\r\n");
		return;
	}

	size_kb = os_strtoul(argv[3], NULL, 0);
	if ((size_kb == 0) || (size_kb > SDTEST_MAX_KB)) {
		os_printf("[SDTEST] write: size_kb must be 1..%u\r\n",
			  (unsigned)SDTEST_MAX_KB);
		return;
	}
	if (argc > 4)
		sync_each_chunk = (os_strtoul(argv[4], NULL, 0) != 0);
	if (argc > 5)
		seed = os_strtoul(argv[5], NULL, 0);

	if (sdtest_fs_busy())
		return;
	if (sdtest_require_mount())
		return;

	sdtest_make_path(argv[2], path, sizeof(path));
	buf = (uint8_t *)os_malloc(SDTEST_IO_CHUNK);
	if (buf == NULL) {
		os_printf("[SDTEST] write: alloc %u bytes failed\r\n",
			  (unsigned)SDTEST_IO_CHUNK);
		return;
	}

	fr = sdtest_write_data(path, size_kb * 1024u, seed, sync_each_chunk,
			       true, buf, NULL, &elapsed_ms);
	sdtest_print_rate("WRITE", path, size_kb * 1024u, elapsed_ms, fr);
	os_free(buf);
}

static void sdtest_cmd_read(int argc, char **argv)
{
	char path[64];
	uint32_t size_kb = 0;
	uint32_t seed = SDTEST_SEED_DEFAULT;
	bool verify;
	uint32_t elapsed_ms = 0;
	uint32_t mismatches = 0;
	uint32_t got = 0;
	uint8_t *buf;
	FRESULT fr;

	if (argc < 3) {
		os_printf("[SDTEST] usage: sdcard read <file> [size_kb] "
			  "[seed]\r\n");
		return;
	}

	if (argc > 3)
		size_kb = os_strtoul(argv[3], NULL, 0);
	if (size_kb > SDTEST_MAX_KB) {
		os_printf("[SDTEST] read: size_kb must be 0..%u\r\n",
			  (unsigned)SDTEST_MAX_KB);
		return;
	}
	verify = (argc > 4);
	if (verify)
		seed = os_strtoul(argv[4], NULL, 0);

	if (sdtest_fs_busy())
		return;
	if (sdtest_require_mount())
		return;

	sdtest_make_path(argv[2], path, sizeof(path));
	buf = (uint8_t *)os_malloc(SDTEST_IO_CHUNK);
	if (buf == NULL) {
		os_printf("[SDTEST] read: alloc %u bytes failed\r\n",
			  (unsigned)SDTEST_IO_CHUNK);
		return;
	}

	fr = sdtest_read_data(path, size_kb * 1024u, verify, seed, buf,
			      NULL, &elapsed_ms, &mismatches, &got);
	sdtest_print_rate("READ", path, got, elapsed_ms, fr);

	if (verify && (fr == FR_OK)) {
		bool ok = (mismatches == 0) &&
			  ((size_kb == 0) || (got == size_kb * 1024u));

		os_printf("[SDTEST] VERIFY result=%s got=%u mismatches=%u\r\n",
			  ok ? "PASS" : "FAIL", (unsigned)got,
			  (unsigned)mismatches);
	}

	os_free(buf);
}

static void sdtest_cmd_rwtest(int argc, char **argv)
{
	uint32_t size_kb = SDTEST_RW_DEFAULT_KB;
	uint32_t rounds = 1;
	uint32_t pass = 0;
	int ret;

	if (argc > 2)
		size_kb = os_strtoul(argv[2], NULL, 0);
	if (argc > 3)
		rounds = os_strtoul(argv[3], NULL, 0);

	if ((size_kb == 0) || (size_kb > SDTEST_MAX_KB) ||
	    (rounds == 0) || (rounds > 100)) {
		os_printf("[SDTEST] usage: sdcard rwtest [size_kb 1..%u] "
			  "[rounds 1..100]\r\n", (unsigned)SDTEST_MAX_KB);
		return;
	}

	if (sdtest_fs_busy())
		return;
	if (sdtest_require_mount())
		return;

	ret = sdtest_rwtest_run(size_kb, rounds, true, &pass);
	os_printf("[SDTEST] RWTEST SUMMARY rounds=%u pass=%u fail=%u "
		  "load=%s\r\n", (unsigned)rounds, (unsigned)pass,
		  (unsigned)(rounds - pass), sdtest_load_on() ? "on" : "off");
	(void)ret;
}

static void sdtest_cmd_randrw(int argc, char **argv)
{
	uint32_t size_kb = SDTEST_RW_DEFAULT_KB;
	uint32_t chunk_kb = SDTEST_RAND_DEFAULT_CHUNK_KB;
	uint32_t rounds = 1;

	if (argc > 2)
		size_kb = os_strtoul(argv[2], NULL, 0);
	if (argc > 3)
		chunk_kb = os_strtoul(argv[3], NULL, 0);
	if (argc > 4)
		rounds = os_strtoul(argv[4], NULL, 0);

	if ((size_kb == 0) || (size_kb > SDTEST_MAX_KB) ||
	    (chunk_kb == 0) || (chunk_kb > SDTEST_RAND_MAX_CHUNK_KB) ||
	    (chunk_kb > size_kb) ||
	    (rounds == 0) || (rounds > SDTEST_RAND_MAX_ROUNDS)) {
		os_printf("[SDTEST] usage: sdcard randrw [size_kb 1..%u] "
			  "[chunk_kb 1..%u] [rounds 1..%u]\r\n",
			  (unsigned)SDTEST_MAX_KB,
			  (unsigned)SDTEST_RAND_MAX_CHUNK_KB,
			  (unsigned)SDTEST_RAND_MAX_ROUNDS);
		return;
	}

	if (sdtest_fs_busy())
		return;
	if (sdtest_require_mount())
		return;

	(void)sdtest_randrw_run(size_kb, chunk_kb, rounds);
}

static void sdtest_cmd_mounttest(int argc, char **argv)
{
	uint32_t rounds = SDTEST_MOUNTTEST_DEFAULT_ROUNDS;
	uint32_t delay_ms = SDTEST_MOUNTTEST_DEFAULT_DELAY;
	uint32_t pass = 0;
	uint32_t fail = 0;
	uint32_t mount_min = 0xffffffffu;
	uint32_t mount_max = 0;
	uint32_t mount_sum = 0;
	uint32_t umount_min = 0xffffffffu;
	uint32_t umount_max = 0;
	uint32_t umount_sum = 0;
	uint32_t i;

	if (argc > 2)
		rounds = os_strtoul(argv[2], NULL, 0);
	if (argc > 3)
		delay_ms = os_strtoul(argv[3], NULL, 0);

	if ((rounds == 0) || (rounds > SDTEST_MOUNTTEST_MAX_ROUNDS) ||
	    (delay_ms > SDTEST_MOUNTTEST_MAX_DELAY)) {
		os_printf("[SDTEST] usage: sdcard mounttest [rounds 1..%u] "
			  "[delay_ms 0..%u]\r\n",
			  (unsigned)SDTEST_MOUNTTEST_MAX_ROUNDS,
			  (unsigned)SDTEST_MOUNTTEST_MAX_DELAY);
		return;
	}

	if (sdtest_fs_busy())
		return;

	/* Start from a known unmounted state so every round includes a real
	 * card init/deinit instead of a no-op mount. */
	(void)sdtest_unmount();

	os_printf("[SDTEST] MOUNTTEST START rounds=%u delay_ms=%u\r\n",
		  (unsigned)rounds, (unsigned)delay_ms);

	for (i = 1; i <= rounds; i++) {
		uint32_t start_ms;
		uint32_t mount_ms;
		uint32_t umount_ms = 0;
		FRESULT mount_fr;
		FRESULT umount_fr = FR_OK;

		start_ms = rtos_get_time();
		mount_fr = sdtest_mount();
		mount_ms = rtos_get_time() - start_ms;

		if (mount_fr != FR_OK) {
			fail++;
			os_printf("[SDTEST] MOUNTTEST round=%u FAIL mount "
				  "fr=%d(%s) ms=%u\r\n", (unsigned)i, mount_fr,
				  sdtest_fr_name(mount_fr), (unsigned)mount_ms);
			(void)sdtest_unmount();
		} else {
			start_ms = rtos_get_time();
			umount_fr = sdtest_unmount();
			umount_ms = rtos_get_time() - start_ms;

			if (umount_fr != FR_OK) {
				fail++;
			} else {
				pass++;
				if (mount_ms < mount_min)
					mount_min = mount_ms;
				if (mount_ms > mount_max)
					mount_max = mount_ms;
				mount_sum += mount_ms;
				if (umount_ms < umount_min)
					umount_min = umount_ms;
				if (umount_ms > umount_max)
					umount_max = umount_ms;
				umount_sum += umount_ms;
			}

			os_printf("[SDTEST] MOUNTTEST round=%u %s mount_ms=%u "
				  "umount_ms=%u umount_fr=%d(%s)\r\n",
				  (unsigned)i,
				  (umount_fr == FR_OK) ? "PASS" : "FAIL",
				  (unsigned)mount_ms, (unsigned)umount_ms,
				  umount_fr, sdtest_fr_name(umount_fr));
		}

		bk_task_wdt_feed();
		if (delay_ms != 0)
			rtos_delay_milliseconds(delay_ms);
	}

	os_printf("[SDTEST] MOUNTTEST SUMMARY rounds=%u pass=%u fail=%u\r\n",
		  (unsigned)rounds, (unsigned)pass, (unsigned)fail);
	if (pass != 0) {
		os_printf("[SDTEST] MOUNTTEST mount_ms min=%u avg=%u max=%u "
			  "umount_ms min=%u avg=%u max=%u\r\n",
			  (unsigned)mount_min,
			  (unsigned)(mount_sum / pass),
			  (unsigned)mount_max,
			  (unsigned)umount_min,
			  (unsigned)(umount_sum / pass),
			  (unsigned)umount_max);
	}
}

static void sdtest_cmd_bench(int argc, char **argv)
{
	uint32_t size_mb = SDTEST_BENCH_DEFAULT_MB;
	uint32_t rounds = SDTEST_BENCH_DEFAULT_ROUNDS;
	uint32_t capacity = sdtest_default_capacity();

	if (argc > 2)
		size_mb = os_strtoul(argv[2], NULL, 0);
	if (argc > 3)
		rounds = os_strtoul(argv[3], NULL, 0);
	if (argc > 4)
		capacity = os_strtoul(argv[4], NULL, 0);

	if ((size_mb == 0) || (size_mb > SDTEST_BENCH_MAX_MB) ||
	    (rounds == 0) || (rounds > SDTEST_BENCH_MAX_ROUNDS) ||
	    (capacity > GPIO_DRIVER_CAPACITY_3)) {
		os_printf("[SDTEST] usage: sdcard bench [size_mb 1..%u] "
			  "[rounds 1..%u] [drive_capacity 0..%u]\r\n",
			  (unsigned)SDTEST_BENCH_MAX_MB,
			  (unsigned)SDTEST_BENCH_MAX_ROUNDS,
			  (unsigned)GPIO_DRIVER_CAPACITY_3);
		return;
	}

	if (sdtest_fs_busy())
		return;
	if (sdtest_require_mount())
		return;

	(void)sdtest_bench_run(size_mb, rounds, capacity, true, NULL);
}

static void sdtest_cmd_soak(int argc, char **argv)
{
	if (argc < 3) {
		os_printf("[SDTEST] usage: sdcard soak start [size_kb] "
			  "[duration_s] | stop | status\r\n");
		return;
	}

	if (os_strcmp(argv[2], "start") == 0) {
		uint32_t size_kb = SDTEST_SOAK_DEFAULT_KB;
		uint32_t duration_s = SDTEST_SOAK_DEFAULT_SEC;
		bk_err_t ret;

		if (argc > 3)
			size_kb = os_strtoul(argv[3], NULL, 0);
		if (argc > 4)
			duration_s = os_strtoul(argv[4], NULL, 0);

		if ((size_kb == 0) || (size_kb > SDTEST_MAX_KB) ||
		    (duration_s > SDTEST_SOAK_MAX_SEC)) {
			os_printf("[SDTEST] soak: size_kb 1..%u, duration_s "
				  "0..%u (0 = until stop)\r\n",
				  (unsigned)SDTEST_MAX_KB,
				  (unsigned)SDTEST_SOAK_MAX_SEC);
			return;
		}

		if (s_soak_run || (s_soak_thread != NULL)) {
			os_printf("[SDTEST] soak already running\r\n");
			return;
		}
		if (sdtest_require_mount())
			return;

		s_soak_size_kb = size_kb;
		s_soak_duration_s = duration_s;
		s_soak_start_ms = rtos_get_time();
		s_soak_elapsed_ms = 0;
		s_soak_iterations = 0;
		s_soak_failures = 0;
		s_soak_total_kb = 0;
		s_soak_write_ms = 0;
		s_soak_read_ms = 0;
		s_soak_stop = false;
		s_soak_run = true;

		ret = rtos_create_thread(&s_soak_thread, 5, "sdtest_soak",
					 sdtest_soak_task, 8192,
					 (beken_thread_arg_t)0);
		if (ret != 0) {
			s_soak_run = false;
			s_soak_thread = NULL;
			os_printf("[SDTEST] soak: create thread failed %d\r\n",
				  ret);
			return;
		}

		os_printf("[SDTEST] SOAK START size_kb=%u duration_s=%u "
			  "load=%s\r\n", (unsigned)size_kb,
			  (unsigned)duration_s,
			  sdtest_load_on() ? "on" : "off");
	} else if (os_strcmp(argv[2], "stop") == 0) {
		uint32_t i;

		if (!s_soak_run && (s_soak_thread == NULL)) {
			os_printf("[SDTEST] soak not running\r\n");
			return;
		}

		s_soak_stop = true;
		for (i = 0; (i < SDTEST_STOP_POLL_CNT) &&
			    (s_soak_thread != NULL); i++)
			rtos_delay_milliseconds(SDTEST_STOP_POLL_MS);

		if (s_soak_thread != NULL)
			os_printf("[SDTEST] soak stop timeout\r\n");
		else
			os_printf("[SDTEST] soak stopped\r\n");
	} else if (os_strcmp(argv[2], "status") == 0) {
		sdtest_soak_status();
	} else {
		os_printf("[SDTEST] usage: sdcard soak start [size_kb] "
			  "[duration_s] | stop | status\r\n");
	}
}

static void sdtest_cmd_load(int argc, char **argv)
{
	if (argc < 3) {
		os_printf("[SDTEST] usage: sdcard load start [work_kb] "
			  "[mode 1=copy 2=fill 3=read] [chunk_kb] | stop | "
			  "status\r\n");
		return;
	}

	if (os_strcmp(argv[2], "start") == 0) {
		uint32_t work_kb = SDLOAD_DEFAULT_WORK_KB;
		uint32_t mode = SDLOAD_MODE_COPY;
		uint32_t chunk_kb = 0;
		int ret;

		if (argc > 3)
			work_kb = os_strtoul(argv[3], NULL, 0);
		if (argc > 4)
			mode = os_strtoul(argv[4], NULL, 0);
		if (argc > 5)
			chunk_kb = os_strtoul(argv[5], NULL, 0);

		ret = sdload_start(work_kb, mode, chunk_kb);
		if (ret != SDLOAD_OK) {
			os_printf("[SDTEST] load start failed ret=%d\r\n", ret);
			return;
		}
		sdtest_load_report("LOAD START");
	} else if (os_strcmp(argv[2], "stop") == 0) {
		int ret = sdload_stop();

		if (ret != SDLOAD_OK)
			os_printf("[SDTEST] load stop failed ret=%d\r\n", ret);
	} else if (os_strcmp(argv[2], "status") == 0) {
		sdtest_load_report("LOAD STATUS");
	} else {
		os_printf("[SDTEST] usage: sdcard load start [work_kb] "
			  "[mode] [chunk_kb] | stop | status\r\n");
	}
}

static void sdtest_cmd_loadtest(int argc, char **argv)
{
	uint32_t size_mb = SDTEST_BENCH_DEFAULT_MB;
	uint32_t rounds = SDTEST_BENCH_DEFAULT_ROUNDS;
	uint32_t work_kb = SDLOAD_DEFAULT_WORK_KB;
	sdtest_bench_result_t bench;
	uint32_t pass = 0;
	bool started = false;
	int ret;

	if (argc > 2)
		size_mb = os_strtoul(argv[2], NULL, 0);
	if (argc > 3)
		rounds = os_strtoul(argv[3], NULL, 0);
	if (argc > 4)
		work_kb = os_strtoul(argv[4], NULL, 0);

	if ((size_mb == 0) || (size_mb > SDTEST_BENCH_MAX_MB) ||
	    (rounds == 0) || (rounds > SDTEST_BENCH_MAX_ROUNDS) ||
	    (work_kb == 0) || (work_kb > SDLOAD_MAX_WORK_KB)) {
		os_printf("[SDTEST] usage: sdcard loadtest [size_mb 1..%u] "
			  "[rounds 1..%u] [work_kb 1..%u]\r\n",
			  (unsigned)SDTEST_BENCH_MAX_MB,
			  (unsigned)SDTEST_BENCH_MAX_ROUNDS,
			  (unsigned)SDLOAD_MAX_WORK_KB);
		return;
	}

	if (sdtest_fs_busy())
		return;
	if (sdtest_require_mount())
		return;

	if (!sdload_is_running()) {
		ret = sdload_start(work_kb, SDLOAD_MODE_COPY, 0);
		if (ret != SDLOAD_OK) {
			os_printf("[SDTEST] loadtest: load start failed %d\r\n",
				  ret);
			return;
		}
		started = true;
		rtos_delay_milliseconds(500);
	} else {
		os_printf("[SDTEST] loadtest: reuse running load\r\n");
	}

	ret = sdtest_bench_run(size_mb, rounds, sdtest_default_capacity(),
			       false, &bench);
	if (ret == 0) {
		os_printf("[SDTEST] LOADTEST VERIFY 1024 KB under load\r\n");
		(void)sdtest_rwtest_run(1024, 1, true, &pass);
	}

	if (started)
		(void)sdload_stop();

	if (ret == 0) {
		bool verify_ok = (pass == 1);

		os_printf("[SDTEST] LOADTEST DONE result=%s "
			  "avg_write_mb_s=%u.%02u avg_read_mb_s=%u.%02u "
			  "verify=%s\r\n",
			  verify_ok ? "PASS" : "FAIL",
			  (unsigned)((bench.write_rate_sum / bench.rounds) / 100u),
			  (unsigned)((bench.write_rate_sum / bench.rounds) % 100u),
			  (unsigned)((bench.read_rate_sum / bench.rounds) / 100u),
			  (unsigned)((bench.read_rate_sum / bench.rounds) % 100u),
			  verify_ok ? "PASS" : "FAIL");
	} else {
		os_printf("[SDTEST] LOADTEST DONE result=FAIL\r\n");
	}
}

static void sdtest_cmd_status(void)
{
	os_printf("[SDTEST] STATUS mounted=%d volume=%s load=%s soak=%s\r\n",
		  s_mounted ? 1 : 0, SDTEST_VOLUME,
		  sdtest_load_on() ? "on" : "off",
		  (s_soak_run || (s_soak_thread != NULL)) ? "on" : "off");
	os_printf("[SDTEST] STATUS clock_hz=%u bus_width=%u high_speed=%u\r\n",
		  (unsigned)CONFIG_SDCARD_CLOCK_FREQ_HZ, SDTEST_BUS_WIDTH,
#if CONFIG_SDCARD_HIGH_SPEED
		  1u
#else
		  0u
#endif
		  );
	sdtest_load_report("LOAD STATUS");
	if (s_soak_run || (s_soak_thread != NULL))
		sdtest_soak_status();
	else
		os_printf("[SDTEST] SOAK STATUS running=0\r\n");
}

static void sdtest_cmd(char *pc_write_buffer, int write_buffer_len, int argc,
		       char **argv)
{
	(void)pc_write_buffer;
	(void)write_buffer_len;

	if (argc < 2) {
		sdtest_help();
		return;
	}

	if (os_strcmp(argv[1], "help") == 0)
		sdtest_help();
	else if (os_strcmp(argv[1], "mount") == 0)
		sdtest_cmd_mount();
	else if (os_strcmp(argv[1], "umount") == 0)
		sdtest_cmd_umount();
	else if (os_strcmp(argv[1], "card") == 0)
		sdtest_cmd_card();
	else if (os_strcmp(argv[1], "info") == 0)
		sdtest_cmd_info();
	else if (os_strcmp(argv[1], "write") == 0)
		sdtest_cmd_write(argc, argv);
	else if (os_strcmp(argv[1], "read") == 0)
		sdtest_cmd_read(argc, argv);
	else if (os_strcmp(argv[1], "rwtest") == 0)
		sdtest_cmd_rwtest(argc, argv);
	else if (os_strcmp(argv[1], "randrw") == 0)
		sdtest_cmd_randrw(argc, argv);
	else if (os_strcmp(argv[1], "mounttest") == 0)
		sdtest_cmd_mounttest(argc, argv);
	else if (os_strcmp(argv[1], "bench") == 0)
		sdtest_cmd_bench(argc, argv);
	else if (os_strcmp(argv[1], "soak") == 0)
		sdtest_cmd_soak(argc, argv);
	else if (os_strcmp(argv[1], "load") == 0)
		sdtest_cmd_load(argc, argv);
	else if (os_strcmp(argv[1], "loadtest") == 0)
		sdtest_cmd_loadtest(argc, argv);
	else if (os_strcmp(argv[1], "status") == 0)
		sdtest_cmd_status();
	else {
		os_printf("[SDTEST] unknown sub command: %s\r\n", argv[1]);
		sdtest_help();
	}
}

static const struct cli_command s_sdtest_commands[] = {
	{"sdcard", "sdcard <mount|umount|card|info|write|read|rwtest|"
		   "randrw|mounttest|bench|soak|load|loadtest|status>",
	 sdtest_cmd},
};

int sdtest_init(void)
{
	return cli_register_commands(s_sdtest_commands,
				     sizeof(s_sdtest_commands) /
				     sizeof(s_sdtest_commands[0]));
}
