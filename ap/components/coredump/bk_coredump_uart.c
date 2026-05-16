#include <stdint.h>
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
static uint32_t s_coredump_uart_locked = 0;

static void bk_coredump_uart_lock(void)
{
    if (s_coredump_uart_locked == 0U) {
        bk_aspl_uart_log_lock();
        s_coredump_uart_locked = 1U;
    }
}

static void bk_coredump_uart_unlock(void)
{
    if (s_coredump_uart_locked != 0U) {
        s_coredump_uart_locked = 0U;
        bk_aspl_uart_log_unlock();
    }
}

static void bk_coredump_uart_init(void)
{
    // take uart lock
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

static void bk_coredump_uart_write_data(uint8_t *data, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        bk_uart_write_byte_unsafe(BK_DUMP_PRINT_UART_PORT, data[i]); // TODO decouple from uart id here
    }
}

static void bk_coredump_uart_write(const char *format, ...)
{
    // write to uart
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    size_t len = strlen(buffer);
    bk_coredump_uart_write_data((uint8_t *)buffer, len);
}

static void coredump_uart_write_memory_base64(const char *name, uint32_t sp, uint32_t fp)
{
    uint8_t base64_buffer[48] = {0};
    uint8_t data[33] = {0};
    int len = 0;
    int base64_len = 0;

    bk_coredump_uart_write(">>>>stack mem dump begin, region: %s, stack_top=%08x, stack end=%08x\r\n",
                           name, sp, fp);
    bk_coredump_uart_write_data((uint8_t *)"\r\n", 2);
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
        bk_coredump_uart_write_data((uint8_t *)base64_buffer, base64_len);
        sp += 32;
    }
    bk_coredump_uart_write_data((uint8_t *)"\r\n", 2);
    bk_coredump_uart_write("<<<<stack mem dump end. region: %s, stack_top=%08x, stack end=%08x\r\n",
                           name, sp, fp);
    bk_coredump_uart_write_data((uint8_t *)"\r\n", 2);
}

static void coredump_uart_write_memory_ascii(const char *name, uint32_t sp, uint32_t fp)
{
    union {
        volatile uint32_t value;
        volatile uint8_t bytes[4];
    } data = {0};

    uint32_t cnt = 0;

    bk_coredump_uart_write(">>>>stack mem dump begin, region: %s, stack_top=%08x, stack end=%08x\r\n",
                            name, sp, fp);
    
    while (sp < fp) {
        if ((cnt & 0x7) == 0) {
            bk_coredump_uart_write_data((uint8_t *)"\r\n", 2);
        }
        data.value = os_get_word(sp);
        bk_coredump_uart_write("%02x %02x %02x %02x ", data.bytes[0], data.bytes[1], data.bytes[2], data.bytes[3]);
        sp += 4;
        cnt++;
    }
    bk_coredump_uart_write_data((uint8_t *)"\r\n\r\n", 4);
    bk_coredump_uart_write("<<<<stack mem dump end. region: %s, stack_top=%08x, stack end=%08x\r\n",
                           name, sp, fp);
    bk_coredump_uart_write_data((uint8_t *)"\r\n", 2);
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
#if CONFIG_WDT_EN
        // bk_wdt_force_feed();
#endif
        len = fp - sp > MEM_DUMP_MAX_LEN ? MEM_DUMP_MAX_LEN : fp - sp;
#if CONFIG_DUMP_UART_MEM_ENCODING_ASCII
        coredump_uart_write_memory_ascii(name, sp, sp + len);
#elif CONFIG_DUMP_UART_MEM_ENCODING_BASE64
        coredump_uart_write_memory_base64(name, sp, sp + len);
#endif
        sp += len;
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
