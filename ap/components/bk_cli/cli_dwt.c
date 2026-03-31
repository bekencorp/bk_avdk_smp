#include "cli.h"
#include "sdkconfig.h"

#if CONFIG_ARCH_CORTEX_M
#include "dwt.h"
extern void bk_delay_us(UINT32 us);
void smp_arch_dwt_trap_write(uint32_t addr, uint32_t data);
void smp_dwt_set_data_write(uint32_t addr);
void smp_arch_dwt_trap_disable(void);

static void dwt_command_usage(void)
{
    BK_LOGD(NULL, "dwtdw data_address\r\n");
    BK_LOGD(NULL, "     data_address: write this data address, hex format\r\n");
    BK_LOGD(NULL, "dwtdd data_address data_value\r\n");
    BK_LOGD(NULL, "     data_address: watch this data address, hex format\r\n");
    BK_LOGD(NULL, "     data_value: match the data value with watching this data address, hex format\r\n");
    BK_LOGD(NULL, "dwtf ms\r\n");
    BK_LOGD(NULL, "     ms: measure window in milliseconds using bk_delay_us()\r\n");
}

static void dwti_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t instruction_addr;

    if (2 != argc) {
        dwt_command_usage();
        return;
    }

    instruction_addr = strtoll(argv[1], NULL, 16);
    dwt_set_instruction_address(instruction_addr);
}

static void dwtf_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t measure_ms;
    uint32_t start, end;
    uint32_t cycles;
    uint64_t freq_hz;
    uint32_t freq_mhz;

    if (2 != argc) {
        dwt_command_usage();
        return;
    }

    measure_ms = strtoul(argv[1], NULL, 10);
    if (measure_ms == 0U) {
        BK_LOGD(NULL, "invalid ms: %s\r\n", argv[1]);
        return;
    }
    uint32_t int_level = rtos_disable_int();
    dwt_init_cycle_counter();

    start = dwt_get_cycle_counter_val();
    bk_delay_us(measure_ms * 1000U);
    end = dwt_get_cycle_counter_val();
    rtos_enable_int(int_level);
    cycles = end - start; /* unsigned handles wrap-around */

    /* freq_hz = cycles / (measure_ms/1000.0) = cycles * 1000 / measure_ms */
    freq_hz = ((uint64_t)cycles * 1000ULL) / (uint64_t)measure_ms;

    freq_mhz = (uint32_t)(freq_hz / 1000000ULL);
    BK_LOGD(NULL, "dwtf: ms=%u, cycles=%u, freq=%u MHz\r\n",
            measure_ms, cycles, freq_mhz);
}

static void dwtdd_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t addr, value;

    if (3 != argc) {
        dwt_command_usage();
        return;
    }

    addr = strtoll(argv[1], NULL, 16);
    value = strtoll(argv[2], NULL, 16);
#ifdef CONFIG_SOC_SMP
    smp_arch_dwt_trap_write(addr, value);
#else
    dwt_conditional_data_watchpoint(addr, value);
#endif
}

#ifdef CONFIG_SOC_SMP

static void dwtdw_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t data_addr;

    if (2 != argc) {
        dwt_command_usage();
        return;
    }

    data_addr = strtoll(argv[1], NULL, 16);
    smp_dwt_set_data_write(data_addr);
}

#else
static void dwtd_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char watch_type;
    uint32_t data_addr;

    if (3 != argc) {
        dwt_command_usage();
        return;
    }

    watch_type = argv[1][0];
    data_addr = strtoll(argv[2], NULL, 16);
    switch (watch_type){
        case 'r':
            dwt_set_data_address_read(data_addr);
            break;

        case 'w':
            dwt_set_data_address_write(data_addr);
            break;

        case 'b':
            dwt_set_data_address_access(data_addr);
            break;
        default:
            dwt_command_usage();
            break;
    }
}

static void dwtdr_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t addr, limit;

    if (3 != argc) {
        dwt_command_usage();
        return;
    }

    addr = strtoll(argv[1], NULL, 16);
    limit = strtoll(argv[2], NULL, 16);

    dwt_set_data_address_range(addr, limit, ACCESS_TYPE_WRITE);
}

#endif // CONFIG_SOC_SMP

#define DWT_CMD_CNT (sizeof(s_dwt_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_dwt_commands[] = {
    {"dwti", "dwt instruction_addr", dwti_Command},
    {"dwtf", "dwtf ms (measure CPU freq)", dwtf_Command},
    {"dwtdd", "dwtdd data_address data_value", dwtdd_Command},
#ifdef CONFIG_SOC_SMP
    {"dwtdw", "dwtdw data_address", dwtdw_Command},
#else
    {"dwtd", "dwtd r/w/b data_address", dwtd_Command},
    {"dwtdr", "dwtdr data_address data_address_limit", dwtdr_Command},
#endif
};

#endif // CONFIG_ARCH_CORTEX_M

