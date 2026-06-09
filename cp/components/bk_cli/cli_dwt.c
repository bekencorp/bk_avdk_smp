#include <stdlib.h>
#include "cli.h"
#include "sdkconfig.h"

#include "dwt.h"

#if CONFIG_SOC_SMP
void smp_arch_dwt_trap_write(uint32_t addr, uint32_t data);
void smp_dwt_set_data_write(uint32_t addr);
void smp_arch_dwt_trap_disable(void);
#endif

static int dwt_parse_hex_u32(const char *str, uint32_t *value)
{
    char *end;
    unsigned long v;

    if ((str == NULL) || (value == NULL)) {
        return -1;
    }

    if ((str[0] != '0') || ((str[1] != 'x') && (str[1] != 'X')) || (str[2] == '\0')) {
        BK_LOGD(NULL, "invalid hex: %s (require 0x prefix, e.g. 0x281a9070)\r\n", str);
        return -1;
    }

    v = strtoul(str, &end, 16);
    if ((end == str + 2) || (*end != '\0')) {
        BK_LOGD(NULL, "invalid hex: %s (require 0x prefix, e.g. 0x281a9070)\r\n", str);
        return -1;
    }

    *value = (uint32_t)v;
    return 0;
}

static void dwt_command_usage(void)
{
#ifdef CONFIG_SOC_SMP
    BK_LOGD(NULL, "dwtdw 0x<data_address>\r\n");
    BK_LOGD(NULL, "     e.g. dwtdw 0x281a9070\r\n");
#else
    BK_LOGD(NULL, "dwtd r/w/b 0x<data_address>\r\n");
    BK_LOGD(NULL, "     r: read, w: write, b: access\r\n");
    BK_LOGD(NULL, "     e.g. dwtd w 0x281a9070\r\n");
    BK_LOGD(NULL, "dwtdr 0x<data_address> 0x<data_address_limit>\r\n");
    BK_LOGD(NULL, "     e.g. dwtdr 0x281a9070 0x281a9080\r\n");
#endif
    BK_LOGD(NULL, "dwtdd 0x<data_address> 0x<data_value>\r\n");
    BK_LOGD(NULL, "     e.g. dwtdd 0x281a9070 0xaabbccdd\r\n");
    BK_LOGD(NULL, "dwti 0x<instruction_addr>\r\n");
    BK_LOGD(NULL, "     e.g. dwti 0x64f691ec\r\n");
}

static void dwti_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t instruction_addr;

    if (2 != argc) {
        dwt_command_usage();
        return;
    }

    if (dwt_parse_hex_u32(argv[1], &instruction_addr) != 0) {
        dwt_command_usage();
        return;
    }
    dwt_set_instruction_address(instruction_addr);
}

static void dwtdd_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t addr, value;

    if (3 != argc) {
        dwt_command_usage();
        return;
    }

    if (dwt_parse_hex_u32(argv[1], &addr) != 0) {
        dwt_command_usage();
        return;
    }
    if (dwt_parse_hex_u32(argv[2], &value) != 0) {
        dwt_command_usage();
        return;
    }
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

    if (dwt_parse_hex_u32(argv[1], &data_addr) != 0) {
        dwt_command_usage();
        return;
    }
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
    if (dwt_parse_hex_u32(argv[2], &data_addr) != 0) {
        dwt_command_usage();
        return;
    }
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

    if (dwt_parse_hex_u32(argv[1], &addr) != 0) {
        dwt_command_usage();
        return;
    }
    if (dwt_parse_hex_u32(argv[2], &limit) != 0) {
        dwt_command_usage();
        return;
    }

    dwt_set_data_address_range(addr, limit, ACCESS_TYPE_WRITE);
}

#endif // CONFIG_SOC_SMP

#define DWT_CMD_CNT (sizeof(s_dwt_commands) / sizeof(struct cli_command))
static const struct cli_command s_dwt_commands[] = {
    {"dwti", "dwti 0x<instruction_addr>", dwti_Command},
    {"dwtdd", "dwtdd 0x<addr> 0x<value>", dwtdd_Command},
#ifdef CONFIG_SOC_SMP
    {"dwtdw", "dwtdw 0x<data_address>", dwtdw_Command},
#else
    {"dwtd", "dwtd r/w/b 0x<data_address>", dwtd_Command},
    {"dwtdr", "dwtdr 0x<addr> 0x<addr_limit>", dwtdr_Command},
#endif
};

int cli_dwt_init(void)
{
    return cli_register_commands(s_dwt_commands, DWT_CMD_CNT);
}
