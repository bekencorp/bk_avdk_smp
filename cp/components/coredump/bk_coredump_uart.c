#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "reg_base.h"
#include "bk_coredump.h"
#include "os/mem.h"
#include "aspl_lock.h"
#include "os/os.h"
#include "bk_uart.h"
#include "memory.h"
#include "common/bk_crc.h"
#include "base_64.h"
#include "wdt_driver.h"
#include "hspl/hspl_res_lock.h"
#include "sys_sw_regs.h"

#if CONFIG_SUPPORT_WWDT
#include "wwdt_driver.h"
#endif

#if CONFIG_DUMP_BY_LOG_UART
void bk_coredump_writer_init(void) __attribute__((alias("bk_coredump_uart_init")));
void bk_coredump_writer_deinit(void) __attribute__((alias("bk_coredump_uart_deinit")));
void bk_coredump_write(const char *format, ...) __attribute__((alias("bk_coredump_uart_write")));
void bk_coredump_write_meta_info(COREDUMP_META_INFO info, void *data) __attribute__((alias("bk_coredump_uart_write_meta_info")));
void bk_coredump_write_prompt(const char *format, ...) __attribute__((alias("bk_coredump_uart_write")));
void bk_coredump_write_registers(const char *name, uint32_t value) __attribute__((alias("bk_coredump_uart_write_registers")));
void bk_coredump_write_memory(const char *name, uint32_t stack_top, uint32_t stack_bottom) __attribute__((alias("bk_coredump_uart_write_memory")));
void bk_coredump_write_prompt_data(uint8_t *data, uint32_t size) __attribute__((alias("bk_coredump_uart_write_data")));
#endif

#define MEM_DUMP_MAX_LEN 4096
#define COREDUMP_WDT_FEED_BYTES 256
#define COREDUMP_UART_MAX_TIMEOUTS 4

static uint32_t s_coredump_uart_locked = 0;
static uint32_t s_coredump_uart_force_write = 0;
static uint32_t s_coredump_uart_timeout_count = 0;
static bk_uart_unsafe_snapshot_t s_coredump_uart_snapshots[COREDUMP_UART_MAX_TIMEOUTS];

static bool bk_coredump_uart_lock(void)
{
    if (s_coredump_uart_locked == 0U) {
        if (bk_hspl_res_must_lock(BK_HSPL_RES_UART_LOG) != BK_OK) {
            if (bk_sys_sw_regs_get_ap_cp_hang_dumping() != 0U) {
                return false;
            }
            s_coredump_uart_force_write = 1U;
            s_coredump_uart_locked = 1U;
            return true;
        }
        s_coredump_uart_force_write = 0U;
        s_coredump_uart_locked = 1U;
    }

    return true;
}

static void bk_coredump_uart_unlock(void)
{
    if (s_coredump_uart_locked != 0U) {
        uint32_t force_write = s_coredump_uart_force_write;

        s_coredump_uart_force_write = 0U;
        s_coredump_uart_locked = 0U;
        if (force_write == 0U) {
            bk_hspl_res_unlock(BK_HSPL_RES_UART_LOG);
        }
    }
}

static void bk_coredump_uart_init(void)
{
    s_coredump_uart_timeout_count = 0U;
    bk_coredump_uart_lock();
}

void bk_coredump_lock(void)
{
    // use uart lock as coredump lock
    bk_coredump_uart_lock();
}

static void bk_coredump_uart_deinit(void)
{
    bk_coredump_uart_unlock();
}

static bool coredump_uart_try_write_data(const uint8_t *data, uint32_t size)
{
    bk_coredump_feed_watchdogs();
    for (uint32_t i = 0; i < size; i++) {
        if ((i != 0U) && ((i & (COREDUMP_WDT_FEED_BYTES - 1U)) == 0U)) {
            bk_coredump_feed_watchdogs();
        }
        if (bk_uart_write_byte_unsafe(BK_DUMP_PRINT_UART_PORT, data[i]) != BK_OK) {
            return false;
        }
    }

    return true;
}

static bool coredump_uart_report_snapshot(const bk_uart_unsafe_snapshot_t *snapshot,
                                          uint32_t timeout_count)
{
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer),
                       "\r\n@UART0-timeout[%u]: time=%llu global=%08x config=%08x "
                       "fifo_cfg=%08x fifo_status=%08x int_en=%08x int_status=%08x "
                       "flow=%08x wake=%08x clk_en=%08x clk_src=%u; "
                       "recovered, retry 4KB\r\n",
                       timeout_count,
                       (unsigned long long)snapshot->timestamp_ms,
                       snapshot->global_ctrl,
                       snapshot->config,
                       snapshot->fifo_config,
                       snapshot->fifo_status,
                       snapshot->int_enable,
                       snapshot->int_status,
                       snapshot->flow_ctrl_config,
                       snapshot->wake_config,
                       snapshot->sys_clk_enable,
                       snapshot->sys_clk_source);

    if (len <= 0) {
        return true;
    }

    uint32_t output_len = (uint32_t)len;
    if (output_len >= sizeof(buffer)) {
        output_len = sizeof(buffer) - 1U;
    }

    return coredump_uart_try_write_data((const uint8_t *)buffer, output_len);
}

static void coredump_uart_handle_timeout(void)
{
    while (s_coredump_uart_timeout_count < COREDUMP_UART_MAX_TIMEOUTS) {
        uint32_t index = s_coredump_uart_timeout_count;
        bk_uart_unsafe_snapshot_t *snapshot = &s_coredump_uart_snapshots[index];

        bk_uart_snapshot_unsafe(BK_DUMP_PRINT_UART_PORT, snapshot);
        s_coredump_uart_timeout_count++;
        bk_uart_recover_unsafe(BK_DUMP_PRINT_UART_PORT, snapshot);
        bk_coredump_feed_watchdogs();

        bool report_complete =
            coredump_uart_report_snapshot(snapshot, s_coredump_uart_timeout_count);
        if (s_coredump_uart_timeout_count >= COREDUMP_UART_MAX_TIMEOUTS) {
            bk_wdt_force_reboot();
        }
        if (report_complete) {
            return;
        }
    }
}

static bool coredump_uart_write_data_checked(const uint8_t *data, uint32_t size)
{
    if (s_coredump_uart_locked == 0U) {
        return false;
    }

    if (coredump_uart_try_write_data(data, size)) {
        return true;
    }

    coredump_uart_handle_timeout();
    return false;
}

static void bk_coredump_uart_write_data(uint8_t *data, uint32_t size)
{
    (void)coredump_uart_write_data_checked(data, size);
}

static bool coredump_uart_vwrite(const char *format, va_list args)
{
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), format, args);
    size_t len = strlen(buffer);
    return coredump_uart_write_data_checked((const uint8_t *)buffer, len);
}

static bool coredump_uart_write_checked(const char *format, ...)
{
    bool result;
    va_list args;

    va_start(args, format);
    result = coredump_uart_vwrite(format, args);
    va_end(args);
    return result;
}

static void bk_coredump_uart_write(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    (void)coredump_uart_vwrite(format, args);
    va_end(args);
}

static bool coredump_uart_write_memory_base64(const char *name, uint32_t sp, uint32_t fp)
{
    uint8_t base64_buffer[48] = {0};
    uint8_t data[33] = {0};
    int len = 0;
    int base64_len = 0;

    if (!coredump_uart_write_checked(
            ">>>>stack mem dump begin, region: %s, stack_top=%08x, stack end=%08x\r\n",
            name, sp, fp) ||
        !coredump_uart_write_data_checked((const uint8_t *)"\r\n", 2U)) {
        return false;
    }

    while (sp < fp) {
        len = fp - sp > 32 ? 32 : fp - sp;
        os_memcpy(data, (uint8_t *)sp, len);
        uint8_t crc = hnd_crc8(data, len, 0xFF);
        data[len] = crc;
        len++;
        base64_encode((uint8_t *)data, len, &base64_len, base64_buffer);
        if (base64_buffer[base64_len - 1] == '\n') {
            base64_buffer[base64_len - 1] = '\r';
            base64_buffer[base64_len] = '\n';
            base64_len++;
        }
        if (!coredump_uart_write_data_checked(base64_buffer, (uint32_t)base64_len)) {
            return false;
        }
        sp += 32;
    }

    if (!coredump_uart_write_data_checked((const uint8_t *)"\r\n", 2U) ||
        !coredump_uart_write_checked(
            "<<<<stack mem dump end. region: %s, stack_top=%08x, stack end=%08x\r\n",
            name, sp, fp) ||
        !coredump_uart_write_data_checked((const uint8_t *)"\r\n", 2U)) {
        return false;
    }

    return true;
}

static bool coredump_uart_write_memory_ascii(const char *name, uint32_t sp, uint32_t fp)
{
    union {
        volatile uint32_t value;
        volatile uint8_t bytes[4];
    } data = {0};

    uint32_t cnt = 0;

    if (!coredump_uart_write_checked(
            ">>>>stack mem dump begin, region: %s, stack_top=%08x, stack end=%08x\r\n",
            name, sp, fp)) {
        return false;
    }
    
    while (sp < fp) {
        if ((cnt & 0x7) == 0) {
            if (!coredump_uart_write_data_checked((const uint8_t *)"\r\n", 2U)) {
                return false;
            }
        }
        data.value = os_get_word(sp);
        if (!coredump_uart_write_checked("%02x %02x %02x %02x ",
                                         data.bytes[0], data.bytes[1],
                                         data.bytes[2], data.bytes[3])) {
            return false;
        }
        sp += 4;
        cnt++;
    }

    if (!coredump_uart_write_data_checked((const uint8_t *)"\r\n\r\n", 4U) ||
        !coredump_uart_write_checked(
            "<<<<stack mem dump end. region: %s, stack_top=%08x, stack end=%08x\r\n",
            name, sp, fp) ||
        !coredump_uart_write_data_checked((const uint8_t *)"\r\n", 2U)) {
        return false;
    }

    return true;
}

static void bk_coredump_uart_write_memory(const char *name, uint32_t stack_top, uint32_t stack_bottom)
{
    uint32_t sp = stack_top;
    uint32_t fp = stack_bottom;
    int len = 0;

    if (stack_bottom <= stack_top) {
        return;
    }

    while (sp < fp) {
        bool block_complete = true;

        bk_coredump_feed_watchdogs();
        len = fp - sp > MEM_DUMP_MAX_LEN ? MEM_DUMP_MAX_LEN : fp - sp;
#if CONFIG_DUMP_UART_MEM_ENCODING_ASCII
        block_complete = coredump_uart_write_memory_ascii(name, sp, sp + len);
#elif CONFIG_DUMP_UART_MEM_ENCODING_BASE64
        block_complete = coredump_uart_write_memory_base64(name, sp, sp + len);
#endif
        if (block_complete) {
            sp += len;
        }
    }
}

static void bk_coredump_uart_write_registers(const char *name, uint32_t value)
{
    bk_coredump_uart_write("%s x 0x%lx\r\n", name, value);
}

static void bk_coredump_uart_write_meta_info(COREDUMP_META_INFO info, void *data)
{
    switch (info) {
        case COREDUMP_REGISTERS_INFO:
            bk_coredump_uart_write("CPU%d Current regs:\r\n", (uint32_t)data);
            break;
        case COREDUMP_BUILD_INFO:
            bk_coredump_uart_write("build time => %s !\r\n", (const char *)data);
            break;
        case COREDUMP_CORE_INFO:
            bk_coredump_uart_write("SMP-Core-id:%u\r\n", (uint32_t)data);
            break;
        case COREDUMP_EXCEPTION_INFO:
            bk_coredump_uart_write("@Dump-reason: %s\r\n", (const char *)data);
            break;
        case COREDUMP_ASSERT_INFO:
            bk_coredump_uart_write("(%d)Assert at: %s:%d\r\n", rtos_get_time(),
                                            ((bk_assert_info_t *)data)->func,
                                            ((bk_assert_info_t *)data)->line);
            break;
        case COREDUMP_TRACEBACK_INFO:
            bk_coredump_uart_write("Traceback:\r\n");
            bk_coredump_uart_write("\tarm-none-eabi-addr2line -piaf -e app.elf ");
            bk_mem_addr_t *lr_trace = (bk_mem_addr_t *)data;
            uint32_t *start = (uint32_t *)(lr_trace->start_addr);
            uint32_t count = lr_trace->size;
            for (uint32_t i = 0; i < count; i++) {
                bk_coredump_uart_write("0x%08x ", start[i]);
            }
            bk_coredump_uart_write("\r\n");
            break;
        default:
            break;
    }
}
