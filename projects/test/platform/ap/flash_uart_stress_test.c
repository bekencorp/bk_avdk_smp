// Copyright 2026 Beken
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

#include <stdbool.h>
#include <stdint.h>
#include <os/os.h>
#include "cli.h"
#include <driver/aon_rtc.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include <driver/uart.h>

#if !CONFIG_UART_RX_DMA
#error "flash_uart_stress requires CONFIG_UART_RX_DMA"
#endif

#define FLASH_UART_STRESS_FRAME_SIZE             (256u)
#define FLASH_UART_STRESS_CRC_OFFSET             (252u)
#define FLASH_UART_STRESS_TX_CHUNK_SIZE           (16u)
#define FLASH_UART_STRESS_TX_YIELD_MS             (1u)
#define FLASH_UART_STRESS_FLASH_SIZE             (300u * 1024u)
#define FLASH_UART_STRESS_SECTOR_SIZE             (4u * 1024u)
#define FLASH_UART_STRESS_PAGE_SIZE               (256u)
#define FLASH_UART_STRESS_FLUSH_LIMIT              (1024u)
#define FLASH_UART_STRESS_DEFAULT_UART_ID         (2u)
#define FLASH_UART_STRESS_DEFAULT_BAUD            (9600u)
#define FLASH_UART_STRESS_DEFAULT_ROUNDS          (100u)
#define FLASH_UART_STRESS_DEFAULT_INTERVAL_MS     (2000u)
#define FLASH_UART_STRESS_MAX_ROUNDS              (10000u)
#define FLASH_UART_STRESS_MAX_INTERVAL_MS         (60000u)
#define FLASH_UART_STRESS_RX_PREP_MS              (25u)
#define FLASH_UART_STRESS_RX_MARGIN_MS            (1000u)
#define FLASH_UART_STRESS_START_TIMEOUT_MS        (5000u)
#define FLASH_UART_STRESS_SEM_TIMEOUT_MS          (1000u)
#define FLASH_UART_STRESS_EXIT_TIMEOUT_MS         (5000u)
#define FLASH_UART_STRESS_STOP_POLL_MS              (1u)
#define FLASH_UART_STRESS_TX_JITTER_MS             (100u)
/*
 * Lower numeric values are higher priority in the Beken RTOS abstraction.
 * RX drains completed frames first, control preserves the AON deadline, and
 * Flash runs whenever both communication tasks are blocked. Pin all test tasks
 * to core 0 so cleanup cannot delete a worker running concurrently on core 1.
 */
#define FLASH_UART_STRESS_RX_PRIO                 (6u)
#define FLASH_UART_STRESS_CONTROL_PRIO            (7u)
#define FLASH_UART_STRESS_FLASH_PRIO              (8u)
#define FLASH_UART_STRESS_CONTROL_STACK           (4096u)
#define FLASH_UART_STRESS_RX_STACK                (2048u)
#define FLASH_UART_STRESS_FLASH_STACK             (3072u)

typedef enum {
	FLASH_UART_STRESS_IDLE = 0,
	FLASH_UART_STRESS_STARTING,
	FLASH_UART_STRESS_RUNNING,
	FLASH_UART_STRESS_ABORT_REQUESTED,
	FLASH_UART_STRESS_STOPPING,
	FLASH_UART_STRESS_STUCK,
	FLASH_UART_STRESS_PASS,
	FLASH_UART_STRESS_FAIL,
	FLASH_UART_STRESS_ABORTED,
} flash_uart_stress_state_t;

typedef struct {
	uint32_t uart_id;
	uint32_t baud_rate;
	uint32_t rounds;
	uint32_t interval_ms;
	uint32_t flash_start;
	uint32_t flash_size;

	volatile uint32_t state;
	volatile bool stop_requested;
	volatile bool user_stop_requested;
	volatile bool fatal_error;
	volatile int rx_len;

	volatile uint32_t tx_frames;
	volatile uint32_t valid_frames;
	volatile uint32_t tx_errors;
	volatile uint32_t rx_length_errors;
	volatile uint32_t sequence_errors;
	volatile uint32_t crc_errors;
	volatile uint32_t data_errors;
	volatile uint32_t unexpected_rx_bytes;
	volatile uint32_t schedule_errors;
	volatile uint32_t max_start_late_ms;
	volatile uint32_t min_tx_interval_ms;
	volatile uint32_t max_tx_interval_ms;
	volatile uint32_t flash_passes;
	volatile uint32_t flash_erase_bytes;
	volatile uint32_t flash_write_bytes;
	volatile uint32_t flash_verify_bytes;
	volatile uint32_t flash_api_errors;
	volatile uint32_t flash_verify_errors;
	volatile uint32_t elapsed_ms;

	beken_thread_t rx_thread;
	beken_thread_t flash_thread;
	beken_semaphore_t rx_request;
	beken_semaphore_t rx_armed;
	beken_semaphore_t rx_done;
	beken_semaphore_t flash_started;
	beken_semaphore_t rx_exited;
	beken_semaphore_t flash_exited;
	uint8_t rx_frame[FLASH_UART_STRESS_FRAME_SIZE];
} flash_uart_stress_ctx_t;

typedef struct {
	beken_thread_t thread;
	beken_semaphore_t run_request;
} flash_uart_stress_manager_t;

static flash_uart_stress_ctx_t s_flash_uart_stress;
static flash_uart_stress_manager_t s_flash_uart_stress_manager;

static uint32_t flash_uart_stress_load_u32(volatile uint32_t *value)
{
	return __atomic_load_n(value, __ATOMIC_ACQUIRE);
}

static void flash_uart_stress_store_u32(volatile uint32_t *value, uint32_t data)
{
	__atomic_store_n(value, data, __ATOMIC_RELEASE);
}

static bool flash_uart_stress_load_bool(volatile bool *value)
{
	return __atomic_load_n(value, __ATOMIC_ACQUIRE);
}

static void flash_uart_stress_store_bool(volatile bool *value, bool data)
{
	__atomic_store_n(value, data, __ATOMIC_RELEASE);
}

static void flash_uart_stress_add_u32(volatile uint32_t *value, uint32_t data)
{
	(void)__atomic_fetch_add(value, data, __ATOMIC_RELAXED);
}

static bool flash_uart_stress_compare_state(uint32_t expected, uint32_t desired)
{
	return __atomic_compare_exchange_n(&s_flash_uart_stress.state, &expected,
					   desired, false, __ATOMIC_ACQ_REL,
					   __ATOMIC_ACQUIRE);
}

static const char *flash_uart_stress_state_name(uint32_t state)
{
	switch ((flash_uart_stress_state_t)state) {
	case FLASH_UART_STRESS_STARTING:
		return "STARTING";
	case FLASH_UART_STRESS_RUNNING:
		return "RUNNING";
	case FLASH_UART_STRESS_ABORT_REQUESTED:
		return "ABORT_REQUESTED";
	case FLASH_UART_STRESS_STOPPING:
		return "STOPPING";
	case FLASH_UART_STRESS_STUCK:
		return "STUCK";
	case FLASH_UART_STRESS_PASS:
		return "PASS";
	case FLASH_UART_STRESS_FAIL:
		return "FAIL";
	case FLASH_UART_STRESS_ABORTED:
		return "ABORTED";
	case FLASH_UART_STRESS_IDLE:
	default:
		return "IDLE";
	}
}

static bool flash_uart_stress_is_active(uint32_t state)
{
	return state == FLASH_UART_STRESS_STARTING ||
	       state == FLASH_UART_STRESS_RUNNING ||
	       state == FLASH_UART_STRESS_ABORT_REQUESTED ||
	       state == FLASH_UART_STRESS_STOPPING ||
	       state == FLASH_UART_STRESS_STUCK;
}

static bool flash_uart_stress_parse_u32(const char *text, uint32_t *value)
{
	uint32_t parsed = 0;

	if (text == NULL || value == NULL || text[0] == '\0') {
		return false;
	}

	for (uint32_t i = 0; text[i] != '\0'; i++) {
		uint32_t digit;

		if (text[i] < '0' || text[i] > '9') {
			return false;
		}
		digit = (uint32_t)(text[i] - '0');
		if (parsed > (0xffffffffu - digit) / 10u) {
			return false;
		}
		parsed = parsed * 10u + digit;
	}

	*value = parsed;
	return true;
}

static uint32_t flash_uart_stress_crc32(const uint8_t *data, uint32_t len)
{
	uint32_t crc = 0xffffffffu;

	for (uint32_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (uint32_t bit = 0; bit < 8; bit++) {
			crc = (crc & 1u) ? ((crc >> 1) ^ 0xedb88320u) : (crc >> 1);
		}
	}

	return ~crc;
}

static void flash_uart_stress_put_u32(uint8_t *data, uint32_t value)
{
	data[0] = (uint8_t)value;
	data[1] = (uint8_t)(value >> 8);
	data[2] = (uint8_t)(value >> 16);
	data[3] = (uint8_t)(value >> 24);
}

static uint32_t flash_uart_stress_get_u32(const uint8_t *data)
{
	return (uint32_t)data[0] |
	       ((uint32_t)data[1] << 8) |
	       ((uint32_t)data[2] << 16) |
	       ((uint32_t)data[3] << 24);
}

static uint8_t flash_uart_stress_next_byte(uint32_t *state)
{
	uint32_t value = *state;

	value ^= value << 13;
	value ^= value >> 17;
	value ^= value << 5;
	*state = value;
	return (uint8_t)(value >> 24);
}

static void flash_uart_stress_build_frame(uint8_t *frame, uint32_t sequence)
{
	uint32_t state = 0x46555354u ^ (sequence * 0x9e3779b9u);
	uint32_t crc;

	frame[0] = 'F';
	frame[1] = 'U';
	frame[2] = 'S';
	frame[3] = 'T';
	flash_uart_stress_put_u32(&frame[4], sequence);
	flash_uart_stress_put_u32(&frame[8], ~sequence);
	for (uint32_t i = 12; i < FLASH_UART_STRESS_CRC_OFFSET; i++) {
		frame[i] = flash_uart_stress_next_byte(&state);
	}
	crc = flash_uart_stress_crc32(frame, FLASH_UART_STRESS_CRC_OFFSET);
	flash_uart_stress_put_u32(&frame[FLASH_UART_STRESS_CRC_OFFSET], crc);
}

static uint32_t flash_uart_stress_wire_time_ms(uint32_t baud_rate)
{
	uint64_t bit_count = (uint64_t)FLASH_UART_STRESS_FRAME_SIZE * 10u;

	return (uint32_t)((bit_count * 1000u + baud_rate - 1u) / baud_rate);
}

static bk_err_t flash_uart_stress_write_frame(flash_uart_stress_ctx_t *ctx,
					       const uint8_t *frame)
{
	for (uint32_t offset = 0; offset < FLASH_UART_STRESS_FRAME_SIZE;
	     offset += FLASH_UART_STRESS_TX_CHUNK_SIZE) {
		uint32_t chunk_len = FLASH_UART_STRESS_FRAME_SIZE - offset;
		bk_err_t ret;

		if (flash_uart_stress_load_bool(&ctx->stop_requested)) {
			return BK_ERR_STATE;
		}
		if (chunk_len > FLASH_UART_STRESS_TX_CHUNK_SIZE) {
			chunk_len = FLASH_UART_STRESS_TX_CHUNK_SIZE;
		}
		ret = bk_uart_write_bytes((uart_id_t)ctx->uart_id,
					  frame + offset, chunk_len);
		if (ret != BK_OK) {
			return ret;
		}
		rtos_delay_milliseconds(FLASH_UART_STRESS_TX_YIELD_MS);
	}

	return BK_OK;
}

static bool flash_uart_stress_wait_until(flash_uart_stress_ctx_t *ctx,
					 uint32_t target_ms)
{
	for (;;) {
		uint32_t now_ms;
		int32_t remaining_ms;
		uint32_t delay_ms;

		if (flash_uart_stress_load_bool(&ctx->stop_requested)) {
			return false;
		}
		now_ms = (uint32_t)bk_aon_rtc_get_ms();
		remaining_ms = (int32_t)(target_ms - now_ms);
		if (remaining_ms <= 0) {
			return true;
		}
		delay_ms = (uint32_t)remaining_ms;
		if (delay_ms > FLASH_UART_STRESS_STOP_POLL_MS) {
			delay_ms = FLASH_UART_STRESS_STOP_POLL_MS;
		}
		rtos_delay_milliseconds(delay_ms);
	}
}

static uint32_t flash_uart_stress_flush_rx(uart_id_t id)
{
	uint8_t data[32];
	int read_len;
	uint32_t total = 0;

	do {
		read_len = bk_uart_read_bytes(id, data, sizeof(data), 5);
		if (read_len > 0) {
			total += (uint32_t)read_len;
		}
	} while (read_len > 0 && total < FLASH_UART_STRESS_FLUSH_LIMIT);

	return total;
}

static int flash_uart_stress_read_full(uart_id_t id, uint8_t *data,
				       uint32_t len, uint32_t timeout_ms)
{
	uint32_t received = 0;
	uint32_t start_ms = (uint32_t)bk_aon_rtc_get_ms();

	while (received < len) {
		uint32_t elapsed_ms =
			(uint32_t)bk_aon_rtc_get_ms() - start_ms;
		uint32_t remaining_ms;
		int read_len;

		if (elapsed_ms >= timeout_ms) {
			break;
		}
		remaining_ms = timeout_ms - elapsed_ms;
		read_len = bk_uart_read_bytes(id, data + received, len - received,
					      remaining_ms);
		if (read_len < 0) {
			if (received == 0) {
				return read_len;
			}
			break;
		}
		if (read_len == 0) {
			break;
		}
		received += (uint32_t)read_len;
	}

	return (int)received;
}

static bool flash_uart_stress_partition_get(bk_logic_partition_t **partition)
{
	bk_logic_partition_t *info;
	uint32_t flash_total;
	uint32_t end;
	bk_err_t ret;

	if (partition == NULL) {
		return false;
	}

	ret = bk_flash_driver_init();
	if (ret != BK_OK) {
		CLI_LOGE("FLASH_UART_STRESS: flash driver init failed ret=%d\r\n", ret);
		return false;
	}
	info = bk_flash_partition_get_info(BK_PARTITION_FLASH_STRESS);
	if (info == NULL) {
		CLI_LOGE("FLASH_UART_STRESS: flash_stress partition not found\r\n");
		return false;
	}
	end = info->partition_start_addr + FLASH_UART_STRESS_FLASH_SIZE;
	flash_total = bk_flash_get_current_total_size();
	if ((info->partition_start_addr & (FLASH_UART_STRESS_SECTOR_SIZE - 1u)) != 0u ||
	    info->partition_length < FLASH_UART_STRESS_FLASH_SIZE ||
	    end < info->partition_start_addr ||
	    (flash_total != 0u && end > flash_total) ||
	    (info->partition_options & PAR_OPT_WRITE_EN) == 0u) {
		CLI_LOGE("FLASH_UART_STRESS: invalid partition start=0x%08x size=0x%08x "
			 "required=0x%08x flash=0x%08x options=0x%x\r\n",
			 info->partition_start_addr, info->partition_length,
			 FLASH_UART_STRESS_FLASH_SIZE, flash_total,
			 info->partition_options);
		return false;
	}

	*partition = info;
	return true;
}

static void flash_uart_stress_fill_flash_page(uint8_t *data, uint32_t pass,
					      uint32_t offset)
{
	uint32_t state = 0x5a17c3e9u ^ (pass * 0x9e3779b9u) ^ offset;

	for (uint32_t i = 0; i < FLASH_UART_STRESS_PAGE_SIZE; i++) {
		data[i] = flash_uart_stress_next_byte(&state);
	}
}

static void flash_uart_stress_report_flash_api_error(flash_uart_stress_ctx_t *ctx,
						     const char *operation,
						     uint32_t address,
						     bk_err_t error)
{
	flash_uart_stress_add_u32(&ctx->flash_api_errors, 1u);
	flash_uart_stress_store_bool(&ctx->fatal_error, true);
	flash_uart_stress_store_bool(&ctx->stop_requested, true);
	CLI_LOGE("FLASH_UART_STRESS: flash %s failed addr=0x%08x ret=%d\r\n",
		 operation, address, error);
}

static void flash_uart_stress_flash_worker(beken_thread_arg_t arg)
{
	flash_uart_stress_ctx_t *ctx = (flash_uart_stress_ctx_t *)arg;
	uint8_t write_data[FLASH_UART_STRESS_PAGE_SIZE] __attribute__((aligned(4)));
	uint8_t read_data[FLASH_UART_STRESS_PAGE_SIZE] __attribute__((aligned(4)));
	bool start_reported = false;
	uint32_t pass = 0;

	while (!flash_uart_stress_load_bool(&ctx->stop_requested)) {
		bool pass_complete = true;

		for (uint32_t sector_offset = 0;
		     sector_offset < ctx->flash_size;
		     sector_offset += FLASH_UART_STRESS_SECTOR_SIZE) {
			uint32_t sector_address = ctx->flash_start + sector_offset;
			bk_err_t ret;

			if (flash_uart_stress_load_bool(&ctx->stop_requested)) {
				pass_complete = false;
				break;
			}

			ret = bk_flash_erase_sector(sector_address);
			if (!start_reported) {
				start_reported = true;
				(void)rtos_set_semaphore(&ctx->flash_started);
			}
			if (ret != BK_OK) {
				flash_uart_stress_report_flash_api_error(ctx, "erase",
								 sector_address, ret);
				pass_complete = false;
				break;
			}

			for (uint32_t page_offset = 0;
			     page_offset < FLASH_UART_STRESS_SECTOR_SIZE;
			     page_offset += FLASH_UART_STRESS_PAGE_SIZE) {
				uint32_t address = sector_address + page_offset;

				ret = bk_flash_read_bytes(address, read_data,
							 FLASH_UART_STRESS_PAGE_SIZE);
				if (ret != BK_OK) {
					flash_uart_stress_report_flash_api_error(ctx,
									 "erase-read",
									 address, ret);
					pass_complete = false;
					break;
				}
				for (uint32_t i = 0; i < FLASH_UART_STRESS_PAGE_SIZE; i++) {
					if (read_data[i] != 0xffu) {
						flash_uart_stress_add_u32(
							&ctx->flash_verify_errors, 1u);
						CLI_LOGE("FLASH_UART_STRESS: erase verify failed "
							 "addr=0x%08x got=0x%02x\r\n",
							 address + i, read_data[i]);
						flash_uart_stress_store_bool(&ctx->fatal_error,
									    true);
						flash_uart_stress_store_bool(&ctx->stop_requested,
									    true);
						pass_complete = false;
						break;
					}
				}
				if (!pass_complete) {
					break;
				}
			}
			if (!pass_complete) {
				break;
			}
			flash_uart_stress_add_u32(&ctx->flash_erase_bytes,
						  FLASH_UART_STRESS_SECTOR_SIZE);

			for (uint32_t page_offset = 0;
			     page_offset < FLASH_UART_STRESS_SECTOR_SIZE;
			     page_offset += FLASH_UART_STRESS_PAGE_SIZE) {
				uint32_t address = sector_address + page_offset;

				flash_uart_stress_fill_flash_page(write_data, pass,
								 sector_offset + page_offset);
				ret = bk_flash_write_bytes(address, write_data,
							  FLASH_UART_STRESS_PAGE_SIZE);
				if (ret != BK_OK) {
					flash_uart_stress_report_flash_api_error(ctx, "write",
									 address, ret);
					pass_complete = false;
					break;
				}
			}
			if (!pass_complete) {
				break;
			}
			flash_uart_stress_add_u32(&ctx->flash_write_bytes,
						  FLASH_UART_STRESS_SECTOR_SIZE);

			for (uint32_t page_offset = 0;
			     page_offset < FLASH_UART_STRESS_SECTOR_SIZE;
			     page_offset += FLASH_UART_STRESS_PAGE_SIZE) {
				uint32_t address = sector_address + page_offset;

				flash_uart_stress_fill_flash_page(write_data, pass,
								 sector_offset + page_offset);
				ret = bk_flash_read_bytes(address, read_data,
							 FLASH_UART_STRESS_PAGE_SIZE);
				if (ret != BK_OK) {
					flash_uart_stress_report_flash_api_error(ctx,
									 "verify-read",
									 address, ret);
					pass_complete = false;
					break;
				}
				if (os_memcmp(write_data, read_data,
					      FLASH_UART_STRESS_PAGE_SIZE) != 0) {
					for (uint32_t mismatch = 0;
					     mismatch < FLASH_UART_STRESS_PAGE_SIZE;
					     mismatch++) {
						if (write_data[mismatch] == read_data[mismatch]) {
							continue;
						}
						flash_uart_stress_add_u32(&ctx->flash_verify_errors,
									  1u);
						CLI_LOGE("FLASH_UART_STRESS: write verify failed "
							 "addr=0x%08x expected=0x%02x got=0x%02x\r\n",
							 address + mismatch, write_data[mismatch],
							 read_data[mismatch]);
						flash_uart_stress_store_bool(&ctx->fatal_error, true);
						flash_uart_stress_store_bool(&ctx->stop_requested,
									     true);
						pass_complete = false;
						break;
					}
					break;
				}
			}
			if (!pass_complete) {
				break;
			}
			flash_uart_stress_add_u32(&ctx->flash_verify_bytes,
						  FLASH_UART_STRESS_SECTOR_SIZE);
			rtos_delay_milliseconds(1);
		}

		if (pass_complete) {
			pass++;
			flash_uart_stress_store_u32(&ctx->flash_passes, pass);
			CLI_LOGI("FLASH_UART_STRESS: flash pass=%u verified=%uKB\r\n",
				 pass,
				 flash_uart_stress_load_u32(&ctx->flash_verify_bytes) / 1024u);
		}
	}

	if (!start_reported) {
		(void)rtos_set_semaphore(&ctx->flash_started);
	}
	(void)rtos_set_semaphore(&ctx->flash_exited);
	rtos_suspend_thread(NULL);
}

static void flash_uart_stress_rx_worker(beken_thread_arg_t arg)
{
	flash_uart_stress_ctx_t *ctx = (flash_uart_stress_ctx_t *)arg;
	uint32_t timeout_ms =
		flash_uart_stress_wire_time_ms(ctx->baud_rate) +
		FLASH_UART_STRESS_RX_MARGIN_MS;

	for (;;) {
		int read_len;

		if (rtos_get_semaphore(&ctx->rx_request, BEKEN_WAIT_FOREVER) != kNoErr) {
			continue;
		}
		if (flash_uart_stress_load_bool(&ctx->stop_requested)) {
			(void)rtos_set_semaphore(&ctx->rx_armed);
			break;
		}

		(void)rtos_set_semaphore(&ctx->rx_armed);
		read_len = flash_uart_stress_read_full(
			(uart_id_t)ctx->uart_id, ctx->rx_frame,
			FLASH_UART_STRESS_FRAME_SIZE, timeout_ms);
		if (bk_uart_disable_rx_interrupt(
			    (uart_id_t)ctx->uart_id) != BK_OK) {
			CLI_LOGE("FLASH_UART_STRESS: uart rx interrupt disable failed id=%u\r\n",
				 ctx->uart_id);
			flash_uart_stress_store_bool(&ctx->fatal_error, true);
			flash_uart_stress_store_bool(&ctx->stop_requested, true);
		}
		__atomic_store_n(&ctx->rx_len, read_len, __ATOMIC_RELEASE);
		(void)rtos_set_semaphore(&ctx->rx_done);
	}

	(void)rtos_set_semaphore(&ctx->rx_exited);
	rtos_suspend_thread(NULL);
}

static bool flash_uart_stress_validate_frame(flash_uart_stress_ctx_t *ctx,
					     const uint8_t *expected,
					     uint32_t sequence)
{
	int read_len = __atomic_load_n(&ctx->rx_len, __ATOMIC_ACQUIRE);
	uint32_t received_crc;
	uint32_t calculated_crc;
	bool valid = true;

	if (read_len != (int)FLASH_UART_STRESS_FRAME_SIZE) {
		flash_uart_stress_add_u32(&ctx->rx_length_errors, 1u);
		CLI_LOGE("FLASH_UART_STRESS: frame=%u length error expected=%u got=%d\r\n",
			 sequence, FLASH_UART_STRESS_FRAME_SIZE, read_len);
		return false;
	}

	if (ctx->rx_frame[0] != 'F' || ctx->rx_frame[1] != 'U' ||
	    ctx->rx_frame[2] != 'S' || ctx->rx_frame[3] != 'T' ||
	    flash_uart_stress_get_u32(&ctx->rx_frame[4]) != sequence ||
	    flash_uart_stress_get_u32(&ctx->rx_frame[8]) != ~sequence) {
		flash_uart_stress_add_u32(&ctx->sequence_errors, 1u);
		valid = false;
	}

	received_crc =
		flash_uart_stress_get_u32(&ctx->rx_frame[FLASH_UART_STRESS_CRC_OFFSET]);
	calculated_crc =
		flash_uart_stress_crc32(ctx->rx_frame, FLASH_UART_STRESS_CRC_OFFSET);
	if (received_crc != calculated_crc) {
		flash_uart_stress_add_u32(&ctx->crc_errors, 1u);
		valid = false;
	}

	if (os_memcmp(expected, ctx->rx_frame, FLASH_UART_STRESS_FRAME_SIZE) != 0) {
		flash_uart_stress_add_u32(&ctx->data_errors, 1u);
		valid = false;
	}

	if (!valid) {
		CLI_LOGE("FLASH_UART_STRESS: frame=%u data error seq=%u crc=%u data=%u\r\n",
			 sequence,
			 flash_uart_stress_load_u32(&ctx->sequence_errors),
			 flash_uart_stress_load_u32(&ctx->crc_errors),
			 flash_uart_stress_load_u32(&ctx->data_errors));
	}
	return valid;
}

static void flash_uart_stress_update_lateness(flash_uart_stress_ctx_t *ctx,
					      uint32_t late_ms)
{
	uint32_t current = flash_uart_stress_load_u32(&ctx->max_start_late_ms);

	if (late_ms > current) {
		flash_uart_stress_store_u32(&ctx->max_start_late_ms, late_ms);
	}
	if (late_ms > FLASH_UART_STRESS_TX_JITTER_MS) {
		flash_uart_stress_add_u32(&ctx->schedule_errors, 1u);
	}
}

static void flash_uart_stress_print_summary(flash_uart_stress_ctx_t *ctx,
					     uint32_t result_state)
{
	CLI_LOGI("FLASH_UART_STRESS SUMMARY: result=%s uart=%u baud=%u "
		 "frames=%u/%u tx_err=%u len_err=%u seq_err=%u crc_err=%u "
		 "data_err=%u extra_rx=%u schedule_err=%u max_late_ms=%u "
		 "tx_interval_min/max=%u/%u\r\n",
		 flash_uart_stress_state_name(result_state),
		 ctx->uart_id, ctx->baud_rate,
		 flash_uart_stress_load_u32(&ctx->valid_frames), ctx->rounds,
		 flash_uart_stress_load_u32(&ctx->tx_errors),
		 flash_uart_stress_load_u32(&ctx->rx_length_errors),
		 flash_uart_stress_load_u32(&ctx->sequence_errors),
		 flash_uart_stress_load_u32(&ctx->crc_errors),
		 flash_uart_stress_load_u32(&ctx->data_errors),
		 flash_uart_stress_load_u32(&ctx->unexpected_rx_bytes),
		 flash_uart_stress_load_u32(&ctx->schedule_errors),
		 flash_uart_stress_load_u32(&ctx->max_start_late_ms),
		 flash_uart_stress_load_u32(&ctx->min_tx_interval_ms),
		 flash_uart_stress_load_u32(&ctx->max_tx_interval_ms));
	CLI_LOGI("FLASH_UART_STRESS FLASH: start=0x%08x size=0x%08x "
		 "passes=%u erase=%uKB write=%uKB verify=%uKB api_err=%u "
		 "verify_err=%u elapsed_ms=%u\r\n",
		 ctx->flash_start, ctx->flash_size,
		 flash_uart_stress_load_u32(&ctx->flash_passes),
		 flash_uart_stress_load_u32(&ctx->flash_erase_bytes) / 1024u,
		 flash_uart_stress_load_u32(&ctx->flash_write_bytes) / 1024u,
		 flash_uart_stress_load_u32(&ctx->flash_verify_bytes) / 1024u,
		 flash_uart_stress_load_u32(&ctx->flash_api_errors),
		 flash_uart_stress_load_u32(&ctx->flash_verify_errors),
		 flash_uart_stress_load_u32(&ctx->elapsed_ms));
}

static void flash_uart_stress_control_worker(beken_thread_arg_t arg)
{
	flash_uart_stress_ctx_t *ctx = (flash_uart_stress_ctx_t *)arg;
	uint8_t tx_frame[FLASH_UART_STRESS_FRAME_SIZE] __attribute__((aligned(4)));
	uart_config_t uart_config;
	bool rx_request_init = false;
	bool rx_armed_init = false;
	bool rx_done_init = false;
	bool flash_started_init = false;
	bool rx_exited_init = false;
	bool flash_exited_init = false;
	bool uart_inited = false;
	uint32_t start_ms = (uint32_t)bk_aon_rtc_get_ms();
	uint32_t next_start_ms;
	uint32_t previous_tx_start_ms = 0;
	uint32_t final_state;
	bool setup_ok = false;

	if (rtos_init_semaphore(&ctx->rx_request, 1) != kNoErr) {
		CLI_LOGE("FLASH_UART_STRESS: rx_request semaphore init failed\r\n");
		goto cleanup;
	}
	rx_request_init = true;
	if (rtos_init_semaphore(&ctx->rx_armed, 1) != kNoErr) {
		CLI_LOGE("FLASH_UART_STRESS: rx_armed semaphore init failed\r\n");
		goto cleanup;
	}
	rx_armed_init = true;
	if (rtos_init_semaphore(&ctx->rx_done, 1) != kNoErr) {
		CLI_LOGE("FLASH_UART_STRESS: rx_done semaphore init failed\r\n");
		goto cleanup;
	}
	rx_done_init = true;
	if (rtos_init_semaphore(&ctx->flash_started, 1) != kNoErr) {
		CLI_LOGE("FLASH_UART_STRESS: flash_started semaphore init failed\r\n");
		goto cleanup;
	}
	flash_started_init = true;
	if (rtos_init_semaphore(&ctx->rx_exited, 1) != kNoErr) {
		CLI_LOGE("FLASH_UART_STRESS: rx_exited semaphore init failed\r\n");
		goto cleanup;
	}
	rx_exited_init = true;
	if (rtos_init_semaphore(&ctx->flash_exited, 1) != kNoErr) {
		CLI_LOGE("FLASH_UART_STRESS: flash_exited semaphore init failed\r\n");
		goto cleanup;
	}
	flash_exited_init = true;

	os_memset(&uart_config, 0, sizeof(uart_config));
	uart_config.baud_rate = ctx->baud_rate;
	uart_config.data_bits = UART_DATA_8_BITS;
	uart_config.parity = UART_PARITY_NONE;
	uart_config.stop_bits = UART_STOP_BITS_1;
	uart_config.flow_ctrl = UART_FLOWCTRL_DISABLE;
	uart_config.src_clk = UART_SCLK_XTAL_26M;
	/* Use bounded PIO bursts so TX cannot overrun the hardware FIFO. */
	uart_config.rx_dma_en = UART_DMA_ENABLE;
	uart_config.tx_dma_en = UART_DMA_DISABLE;
	if (bk_uart_init((uart_id_t)ctx->uart_id, &uart_config) != BK_OK) {
		CLI_LOGE("FLASH_UART_STRESS: uart init failed id=%u\r\n", ctx->uart_id);
		goto cleanup;
	}
	uart_inited = true;
	if (!bk_uart_is_rx_dma_enabled((uart_id_t)ctx->uart_id)) {
		CLI_LOGE("FLASH_UART_STRESS: uart rx dma unavailable id=%u\r\n",
			 ctx->uart_id);
		goto cleanup;
	}
	/*
	 * Keep the RX DMA request watermark low so each bounded TX burst is
	 * transferred to memory promptly.
	 */
	if (bk_uart_set_rx_full_threshold((uart_id_t)ctx->uart_id, 1u) != BK_OK) {
		CLI_LOGE("FLASH_UART_STRESS: uart rx threshold setup failed id=%u\r\n",
			 ctx->uart_id);
		goto cleanup;
	}
	if (bk_uart_disable_rx_interrupt((uart_id_t)ctx->uart_id) != BK_OK) {
		CLI_LOGE("FLASH_UART_STRESS: uart rx interrupt disable failed id=%u\r\n",
			 ctx->uart_id);
		goto cleanup;
	}
	(void)flash_uart_stress_flush_rx((uart_id_t)ctx->uart_id);

	if (rtos_core0_create_thread(&ctx->rx_thread, FLASH_UART_STRESS_RX_PRIO,
				     "flash_uart_rx",
				     flash_uart_stress_rx_worker,
				     FLASH_UART_STRESS_RX_STACK,
				     (beken_thread_arg_t)ctx) != kNoErr) {
		CLI_LOGE("FLASH_UART_STRESS: rx thread create failed\r\n");
		goto cleanup;
	}
	if (rtos_core0_create_thread(&ctx->flash_thread,
				     FLASH_UART_STRESS_FLASH_PRIO,
				     "flash_uart_flash",
				     flash_uart_stress_flash_worker,
				     FLASH_UART_STRESS_FLASH_STACK,
				     (beken_thread_arg_t)ctx) != kNoErr) {
		CLI_LOGE("FLASH_UART_STRESS: flash thread create failed\r\n");
		goto cleanup;
	}
	if (rtos_get_semaphore(&ctx->flash_started,
			       FLASH_UART_STRESS_START_TIMEOUT_MS) != kNoErr ||
	    flash_uart_stress_load_bool(&ctx->fatal_error)) {
		CLI_LOGE("FLASH_UART_STRESS: flash worker start failed\r\n");
		goto cleanup;
	}

	setup_ok = true;
	if (!flash_uart_stress_compare_state(FLASH_UART_STRESS_STARTING,
					     FLASH_UART_STRESS_RUNNING)) {
		goto cleanup;
	}
	CLI_LOGI("FLASH_UART_STRESS START: uart=%u baud=%u frames=%u interval_ms=%u "
		 "flash=0x%08x+0x%08x mode=rx-dma/tx-pio16-aon "
		 "(GPIO41 TX <-> GPIO40 RX)\r\n",
		 ctx->uart_id, ctx->baud_rate, ctx->rounds, ctx->interval_ms,
		 ctx->flash_start, ctx->flash_size);

	next_start_ms = (uint32_t)bk_aon_rtc_get_ms() + 100u;
	for (uint32_t sequence = 0; sequence < ctx->rounds; sequence++) {
		uint32_t now_ms;
		uint32_t prep_ms = next_start_ms - FLASH_UART_STRESS_RX_PREP_MS;
		int32_t start_late_ms;
		bk_err_t write_ret;
		bool valid;

		if (flash_uart_stress_load_bool(&ctx->stop_requested)) {
			break;
		}

		flash_uart_stress_build_frame(tx_frame, sequence);
		if (!flash_uart_stress_wait_until(ctx, prep_ms)) {
			break;
		}
		if (rtos_set_semaphore(&ctx->rx_request) != kNoErr) {
			if (flash_uart_stress_load_bool(&ctx->stop_requested)) {
				break;
			}
			flash_uart_stress_add_u32(&ctx->rx_length_errors, 1u);
			CLI_LOGE("FLASH_UART_STRESS: frame=%u receiver request failed\r\n",
				 sequence);
			flash_uart_stress_store_bool(&ctx->fatal_error, true);
			break;
		}
		if (rtos_get_semaphore(&ctx->rx_armed,
				       FLASH_UART_STRESS_SEM_TIMEOUT_MS) != kNoErr) {
			if (flash_uart_stress_load_bool(&ctx->stop_requested)) {
				break;
			}
			flash_uart_stress_add_u32(&ctx->rx_length_errors, 1u);
			CLI_LOGE("FLASH_UART_STRESS: frame=%u receiver arm failed\r\n",
				 sequence);
			flash_uart_stress_store_bool(&ctx->fatal_error, true);
			break;
		}
		if (flash_uart_stress_load_bool(&ctx->stop_requested) ||
		    !flash_uart_stress_wait_until(ctx, next_start_ms)) {
			break;
		}
		now_ms = (uint32_t)bk_aon_rtc_get_ms();
		start_late_ms = (int32_t)(now_ms - next_start_ms);
		flash_uart_stress_update_lateness(
			ctx, start_late_ms > 0 ? (uint32_t)start_late_ms : 0u);
		if (previous_tx_start_ms != 0u) {
			uint32_t tx_interval_ms = now_ms - previous_tx_start_ms;
			uint32_t min_interval_ms =
				flash_uart_stress_load_u32(&ctx->min_tx_interval_ms);
			uint32_t max_interval_ms =
				flash_uart_stress_load_u32(&ctx->max_tx_interval_ms);

			if (min_interval_ms == 0u ||
			    tx_interval_ms < min_interval_ms) {
				flash_uart_stress_store_u32(
					&ctx->min_tx_interval_ms, tx_interval_ms);
			}
			if (tx_interval_ms > max_interval_ms) {
				flash_uart_stress_store_u32(
					&ctx->max_tx_interval_ms, tx_interval_ms);
			}
		}
		previous_tx_start_ms = now_ms;

		if (flash_uart_stress_load_bool(&ctx->stop_requested)) {
			break;
		}
		write_ret = flash_uart_stress_write_frame(ctx, tx_frame);
		if (write_ret != BK_OK) {
			flash_uart_stress_add_u32(&ctx->tx_errors, 1u);
			CLI_LOGE("FLASH_UART_STRESS: frame=%u write failed ret=%d\r\n",
				 sequence, write_ret);
		} else {
			flash_uart_stress_add_u32(&ctx->tx_frames, 1u);
		}

		if (rtos_get_semaphore(
			    &ctx->rx_done,
			    flash_uart_stress_wire_time_ms(ctx->baud_rate) +
				    FLASH_UART_STRESS_RX_MARGIN_MS +
				    FLASH_UART_STRESS_SEM_TIMEOUT_MS) != kNoErr) {
			flash_uart_stress_add_u32(&ctx->rx_length_errors, 1u);
			CLI_LOGE("FLASH_UART_STRESS: frame=%u receiver completion timeout\r\n",
				 sequence);
			flash_uart_stress_store_bool(&ctx->fatal_error, true);
			break;
		}

		valid = write_ret == BK_OK &&
			flash_uart_stress_validate_frame(ctx, tx_frame, sequence);
		if (valid) {
			flash_uart_stress_add_u32(&ctx->valid_frames, 1u);
		}
		if (!valid || ((sequence + 1u) % 10u) == 0u ||
		    sequence + 1u == ctx->rounds) {
			CLI_LOGI("FLASH_UART_STRESS: progress=%u/%u valid=%u flash_pass=%u\r\n",
				 sequence + 1u, ctx->rounds,
				 flash_uart_stress_load_u32(&ctx->valid_frames),
				 flash_uart_stress_load_u32(&ctx->flash_passes));
		}

		/* Never send catch-up frames closer than the configured interval. */
		next_start_ms = now_ms + ctx->interval_ms;
	}

	if (setup_ok &&
	    !flash_uart_stress_load_bool(&ctx->stop_requested) &&
	    flash_uart_stress_load_u32(&ctx->tx_frames) == ctx->rounds) {
		uint32_t unexpected;

		rtos_delay_milliseconds(20);
		unexpected =
			flash_uart_stress_flush_rx((uart_id_t)ctx->uart_id);
		if (unexpected > 0u) {
			flash_uart_stress_add_u32(&ctx->unexpected_rx_bytes,
						  unexpected);
			CLI_LOGE("FLASH_UART_STRESS: unexpected trailing RX bytes=%u\r\n",
				 unexpected);
		}
	}

cleanup:
	(void)flash_uart_stress_compare_state(FLASH_UART_STRESS_RUNNING,
					      FLASH_UART_STRESS_STOPPING);
	(void)flash_uart_stress_compare_state(FLASH_UART_STRESS_STARTING,
					      FLASH_UART_STRESS_STOPPING);
	flash_uart_stress_store_bool(&ctx->stop_requested, true);
	if (rx_request_init) {
		(void)rtos_set_semaphore(&ctx->rx_request);
	}
	if (ctx->flash_thread != NULL) {
		if (flash_exited_init &&
		    rtos_get_semaphore(&ctx->flash_exited,
				       FLASH_UART_STRESS_EXIT_TIMEOUT_MS) != kNoErr) {
			CLI_LOGE("FLASH_UART_STRESS: flash worker did not exit; "
				 "resources retained\r\n");
			goto cleanup_stuck;
		}
		(void)rtos_delete_thread(&ctx->flash_thread);
		ctx->flash_thread = NULL;
	}
	if (ctx->rx_thread != NULL) {
		if (rx_exited_init &&
		    rtos_get_semaphore(&ctx->rx_exited,
				       FLASH_UART_STRESS_EXIT_TIMEOUT_MS) != kNoErr) {
			CLI_LOGE("FLASH_UART_STRESS: rx worker did not exit; "
				 "resources retained\r\n");
			goto cleanup_stuck;
		}
		(void)rtos_delete_thread(&ctx->rx_thread);
		ctx->rx_thread = NULL;
	}
	if (uart_inited) {
		(void)bk_uart_deinit((uart_id_t)ctx->uart_id);
	}
	if (flash_exited_init) {
		(void)rtos_deinit_semaphore(&ctx->flash_exited);
		ctx->flash_exited = NULL;
	}
	if (rx_exited_init) {
		(void)rtos_deinit_semaphore(&ctx->rx_exited);
		ctx->rx_exited = NULL;
	}
	if (flash_started_init) {
		(void)rtos_deinit_semaphore(&ctx->flash_started);
		ctx->flash_started = NULL;
	}
	if (rx_done_init) {
		(void)rtos_deinit_semaphore(&ctx->rx_done);
		ctx->rx_done = NULL;
	}
	if (rx_armed_init) {
		(void)rtos_deinit_semaphore(&ctx->rx_armed);
		ctx->rx_armed = NULL;
	}
	if (rx_request_init) {
		(void)rtos_deinit_semaphore(&ctx->rx_request);
		ctx->rx_request = NULL;
	}

	flash_uart_stress_store_u32(
		&ctx->elapsed_ms, (uint32_t)bk_aon_rtc_get_ms() - start_ms);
	if (flash_uart_stress_load_bool(&ctx->user_stop_requested) ||
	    flash_uart_stress_load_u32(&ctx->state) ==
		    FLASH_UART_STRESS_ABORT_REQUESTED) {
		final_state = FLASH_UART_STRESS_ABORTED;
	} else if (setup_ok &&
		   !flash_uart_stress_load_bool(&ctx->fatal_error) &&
		   flash_uart_stress_load_u32(&ctx->tx_frames) == ctx->rounds &&
		   flash_uart_stress_load_u32(&ctx->valid_frames) == ctx->rounds &&
		   flash_uart_stress_load_u32(&ctx->tx_errors) == 0u &&
		   flash_uart_stress_load_u32(&ctx->rx_length_errors) == 0u &&
		   flash_uart_stress_load_u32(&ctx->sequence_errors) == 0u &&
		   flash_uart_stress_load_u32(&ctx->crc_errors) == 0u &&
		   flash_uart_stress_load_u32(&ctx->data_errors) == 0u &&
		   flash_uart_stress_load_u32(&ctx->unexpected_rx_bytes) == 0u &&
		   flash_uart_stress_load_u32(&ctx->schedule_errors) == 0u &&
		   (ctx->rounds <= 1u ||
		    (flash_uart_stress_load_u32(&ctx->min_tx_interval_ms) +
			     FLASH_UART_STRESS_TX_JITTER_MS >= ctx->interval_ms &&
		     flash_uart_stress_load_u32(&ctx->max_tx_interval_ms) <=
			     ctx->interval_ms + FLASH_UART_STRESS_TX_JITTER_MS)) &&
		   flash_uart_stress_load_u32(&ctx->flash_passes) > 0u &&
		   flash_uart_stress_load_u32(&ctx->flash_api_errors) == 0u &&
		   flash_uart_stress_load_u32(&ctx->flash_verify_errors) == 0u) {
		final_state = FLASH_UART_STRESS_PASS;
	} else {
		final_state = FLASH_UART_STRESS_FAIL;
	}
	flash_uart_stress_print_summary(ctx, final_state);
	flash_uart_stress_store_u32(&ctx->state, final_state);
	return;

cleanup_stuck:
	flash_uart_stress_store_bool(&ctx->fatal_error, true);
	flash_uart_stress_store_u32(
		&ctx->elapsed_ms, (uint32_t)bk_aon_rtc_get_ms() - start_ms);
	CLI_LOGE("FLASH_UART_STRESS: cleanup is stuck; reboot is required\r\n");
	flash_uart_stress_print_summary(ctx, FLASH_UART_STRESS_STUCK);
	flash_uart_stress_store_u32(&ctx->state, FLASH_UART_STRESS_STUCK);
}

static void flash_uart_stress_manager_worker(beken_thread_arg_t arg)
{
	flash_uart_stress_ctx_t *ctx = (flash_uart_stress_ctx_t *)arg;

	for (;;) {
		if (rtos_get_semaphore(&s_flash_uart_stress_manager.run_request,
				       BEKEN_WAIT_FOREVER) != kNoErr) {
			continue;
		}
		flash_uart_stress_control_worker((beken_thread_arg_t)ctx);
	}
}

static void flash_uart_stress_print_info(void)
{
	bk_logic_partition_t *partition = NULL;

	if (!flash_uart_stress_partition_get(&partition)) {
		return;
	}
	CLI_LOGI("FLASH_UART_STRESS INFO: partition=%s id=%u start=0x%08x "
		 "size=0x%08x end=0x%08x required=0x%08x\r\n",
		 partition->partition_description, (uint32_t)BK_PARTITION_FLASH_STRESS,
		 partition->partition_start_addr, partition->partition_length,
		 partition->partition_start_addr + partition->partition_length,
		 FLASH_UART_STRESS_FLASH_SIZE);
	CLI_LOGI("FLASH_UART_STRESS INFO: destructive scratch partition; "
		 "do not store production data here\r\n");
	CLI_LOGI("FLASH_UART_STRESS INFO: standalone CP command: "
		 "flash_test W 0x%x 0x%x INC\r\n",
		 partition->partition_start_addr, FLASH_UART_STRESS_FLASH_SIZE);
}

static void flash_uart_stress_print_status(void)
{
	flash_uart_stress_ctx_t *ctx = &s_flash_uart_stress;
	uint32_t state = flash_uart_stress_load_u32(&ctx->state);

	CLI_LOGI("FLASH_UART_STRESS STATUS: state=%s stop=%u uart=%u baud=%u "
		 "frames=%u/%u flash_pass=%u api_err=%u verify_err=%u\r\n",
		 flash_uart_stress_state_name(state),
		 flash_uart_stress_load_bool(&ctx->stop_requested) ? 1u : 0u,
		 ctx->uart_id, ctx->baud_rate,
		 flash_uart_stress_load_u32(&ctx->valid_frames), ctx->rounds,
		 flash_uart_stress_load_u32(&ctx->flash_passes),
		 flash_uart_stress_load_u32(&ctx->flash_api_errors),
		 flash_uart_stress_load_u32(&ctx->flash_verify_errors));
	if (!flash_uart_stress_is_active(state) &&
	    state != FLASH_UART_STRESS_IDLE) {
		flash_uart_stress_print_summary(ctx, state);
	}
}

static bool flash_uart_stress_ensure_manager(void)
{
	if (s_flash_uart_stress_manager.thread != NULL) {
		return true;
	}
	if (rtos_init_semaphore(&s_flash_uart_stress_manager.run_request, 1) !=
	    kNoErr) {
		CLI_LOGE("FLASH_UART_STRESS: manager semaphore init failed\r\n");
		return false;
	}
	if (rtos_core0_create_thread(&s_flash_uart_stress_manager.thread,
				     FLASH_UART_STRESS_CONTROL_PRIO,
				     "flash_uart_mgr",
				     flash_uart_stress_manager_worker,
				     FLASH_UART_STRESS_CONTROL_STACK,
				     (beken_thread_arg_t)&s_flash_uart_stress) !=
	    kNoErr) {
		s_flash_uart_stress_manager.thread = NULL;
		(void)rtos_deinit_semaphore(
			&s_flash_uart_stress_manager.run_request);
		s_flash_uart_stress_manager.run_request = NULL;
		CLI_LOGE("FLASH_UART_STRESS: manager thread create failed\r\n");
		return false;
	}
	return true;
}

static bool flash_uart_stress_start(int argc, char **argv)
{
	flash_uart_stress_ctx_t *ctx = &s_flash_uart_stress;
	bk_logic_partition_t *partition = NULL;
	uint32_t uart_id = FLASH_UART_STRESS_DEFAULT_UART_ID;
	uint32_t baud_rate = FLASH_UART_STRESS_DEFAULT_BAUD;
	uint32_t rounds = FLASH_UART_STRESS_DEFAULT_ROUNDS;
	uint32_t interval_ms = FLASH_UART_STRESS_DEFAULT_INTERVAL_MS;
	uint32_t minimum_interval_ms;
	uint32_t old_state = flash_uart_stress_load_u32(&ctx->state);

	if (flash_uart_stress_is_active(old_state)) {
		CLI_LOGE("FLASH_UART_STRESS: test already active state=%s\r\n",
			 flash_uart_stress_state_name(old_state));
		return false;
	}
	if (argc > 6 ||
	    (argc >= 3 && !flash_uart_stress_parse_u32(argv[2], &uart_id)) ||
	    (argc >= 4 && !flash_uart_stress_parse_u32(argv[3], &baud_rate)) ||
	    (argc >= 5 && !flash_uart_stress_parse_u32(argv[4], &rounds)) ||
	    (argc >= 6 && !flash_uart_stress_parse_u32(argv[5], &interval_ms))) {
		CLI_LOGE("FLASH_UART_STRESS: usage: flash_uart_stress start "
			 "[uart_id] [baud] [rounds] [interval_ms]\r\n");
		return false;
	}
	if (uart_id != FLASH_UART_STRESS_DEFAULT_UART_ID ||
	    uart_id == (uint32_t)CONFIG_UART_PRINT_PORT) {
		CLI_LOGE("FLASH_UART_STRESS: uart id=%u is invalid; use UART2 "
			 "(GPIO41 TX/GPIO40 RX)\r\n",
			 uart_id);
		return false;
	}
	if (baud_rate != 9600u && baud_rate != 38400u &&
	    baud_rate != 115200u) {
		CLI_LOGE("FLASH_UART_STRESS: baud must be 9600, 38400, or 115200\r\n");
		return false;
	}
	if (rounds == 0u || rounds > FLASH_UART_STRESS_MAX_ROUNDS) {
		CLI_LOGE("FLASH_UART_STRESS: rounds must be 1..%u\r\n",
			 FLASH_UART_STRESS_MAX_ROUNDS);
		return false;
	}
	minimum_interval_ms =
		flash_uart_stress_wire_time_ms(baud_rate) + 500u;
	if (interval_ms < minimum_interval_ms ||
	    interval_ms > FLASH_UART_STRESS_MAX_INTERVAL_MS) {
		CLI_LOGE("FLASH_UART_STRESS: interval_ms must be %u..%u at baud=%u\r\n",
			 minimum_interval_ms, FLASH_UART_STRESS_MAX_INTERVAL_MS,
			 baud_rate);
		return false;
	}
	if (bk_uart_is_in_used((uart_id_t)uart_id)) {
		CLI_LOGE("FLASH_UART_STRESS: uart id=%u is already in use\r\n", uart_id);
		return false;
	}
	if (!flash_uart_stress_partition_get(&partition)) {
		return false;
	}
	if (!flash_uart_stress_ensure_manager()) {
		return false;
	}
	os_memset(ctx, 0, sizeof(*ctx));
	ctx->uart_id = uart_id;
	ctx->baud_rate = baud_rate;
	ctx->rounds = rounds;
	ctx->interval_ms = interval_ms;
	ctx->flash_start = partition->partition_start_addr;
	ctx->flash_size = FLASH_UART_STRESS_FLASH_SIZE;
	flash_uart_stress_store_u32(&ctx->state, FLASH_UART_STRESS_STARTING);

	if (rtos_set_semaphore(&s_flash_uart_stress_manager.run_request) !=
	    kNoErr) {
		flash_uart_stress_store_u32(&ctx->state, FLASH_UART_STRESS_FAIL);
		CLI_LOGE("FLASH_UART_STRESS: manager request failed\r\n");
		return false;
	}

	CLI_LOGI("FLASH_UART_STRESS: start accepted; use "
		 "'ap_cmd flash_uart_stress status' for progress\r\n");
	return true;
}

static void flash_uart_stress_cli(char *pcWriteBuffer, int xWriteBufferLen,
				  int argc, char **argv)
{
	bool success = true;

	(void)xWriteBufferLen;
	if (argc < 2) {
		CLI_LOGI("Usage:\r\n");
		CLI_LOGI("  flash_uart_stress start [uart_id] [baud] [rounds] "
			 "[interval_ms]\r\n");
		CLI_LOGI("  flash_uart_stress status|stop|info\r\n");
		success = false;
	} else if (os_strcmp(argv[1], "start") == 0) {
		success = flash_uart_stress_start(argc, argv);
	} else if (os_strcmp(argv[1], "status") == 0) {
		flash_uart_stress_print_status();
	} else if (os_strcmp(argv[1], "info") == 0) {
		flash_uart_stress_print_info();
	} else if (os_strcmp(argv[1], "stop") == 0) {
		uint32_t state =
			flash_uart_stress_load_u32(&s_flash_uart_stress.state);
		bool accepted = false;

		if (state == FLASH_UART_STRESS_STARTING) {
			accepted = flash_uart_stress_compare_state(
				FLASH_UART_STRESS_STARTING,
				FLASH_UART_STRESS_ABORT_REQUESTED);
		} else if (state == FLASH_UART_STRESS_RUNNING) {
			accepted = flash_uart_stress_compare_state(
				FLASH_UART_STRESS_RUNNING,
				FLASH_UART_STRESS_ABORT_REQUESTED);
		} else if (state == FLASH_UART_STRESS_ABORT_REQUESTED) {
			accepted = true;
		}
		if (!accepted) {
			state =
				flash_uart_stress_load_u32(&s_flash_uart_stress.state);
			if (state == FLASH_UART_STRESS_STUCK) {
				CLI_LOGE("FLASH_UART_STRESS: cleanup is stuck; "
					 "reboot is required\r\n");
			} else {
				CLI_LOGE("FLASH_UART_STRESS: no active test state=%s\r\n",
					 flash_uart_stress_state_name(state));
			}
			success = false;
		} else {
			flash_uart_stress_store_bool(
				&s_flash_uart_stress.user_stop_requested, true);
			flash_uart_stress_store_bool(
				&s_flash_uart_stress.stop_requested, true);
			CLI_LOGI("FLASH_UART_STRESS: stop requested\r\n");
		}
	} else {
		CLI_LOGE("FLASH_UART_STRESS: unknown action=%s\r\n", argv[1]);
		success = false;
	}

	if (pcWriteBuffer != NULL) {
		const char *response =
			success ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR;
		os_memcpy(pcWriteBuffer, response, os_strlen(response));
	}
}

DRV_CLI_CMD_EXPORT static const struct cli_command s_flash_uart_stress_commands[] = {
	{"flash_uart_stress",
	 "flash_uart_stress start [uart_id] [baud] [rounds] [interval_ms] | status | stop | info",
	 flash_uart_stress_cli},
};
