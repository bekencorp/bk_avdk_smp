#include <string.h>
#include <common/sys_config.h>
#include "bk_cli.h"
#include "cli.h"
#include "driver/gpio.h"
#include "gpio_driver.h"
#include "sys_hal.h"

extern void cli_mac802154(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void txdtm_mac80154_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void rxdtm_mac80154_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void thread_mac802154_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void mac802154_diag_debug_send_to_internal(uint16_t diag_no);
extern void cli_802154(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

static void cli_mac802154_diag_help(void)
{
    CLI_LOGI("mac802154_diag set mac [diag_no]\r\n");
    CLI_LOGI("--enable MAC diagnostics GPIO output\r\n");
    CLI_LOGI("--diag_no - HW diagnostics no\r\n");
}

static void dbg_enable_debug_gpio_mac802154(void)
{
    gpio_dev_unmap(GPIO_6);
    gpio_dev_map(GPIO_6, GPIO_DEV_DEBUG0);
    gpio_dev_unmap(GPIO_7);
    gpio_dev_map(GPIO_7, GPIO_DEV_DEBUG1);
    gpio_dev_unmap(GPIO_8);
    gpio_dev_map(GPIO_8, GPIO_DEV_DEBUG2);
    gpio_dev_unmap(GPIO_9);
    gpio_dev_map(GPIO_9, GPIO_DEV_DEBUG3);
    gpio_dev_unmap(GPIO_0);
    gpio_dev_map(GPIO_0, GPIO_DEV_DEBUG4);
    gpio_dev_unmap(GPIO_1);
    gpio_dev_map(GPIO_1, GPIO_DEV_DEBUG5);
    gpio_dev_unmap(GPIO_12);
    gpio_dev_map(GPIO_12, GPIO_DEV_DEBUG6);
    gpio_dev_unmap(GPIO_13);
    gpio_dev_map(GPIO_13, GPIO_DEV_DEBUG7);

    gpio_dev_unmap(GPIO_14);
    gpio_dev_map(GPIO_14, GPIO_DEV_DEBUG8);
    gpio_dev_unmap(GPIO_15);
    gpio_dev_map(GPIO_15, GPIO_DEV_DEBUG9);
    gpio_dev_unmap(GPIO_16);
    gpio_dev_map(GPIO_16, GPIO_DEV_DEBUG10);
    gpio_dev_unmap(GPIO_17);
    gpio_dev_map(GPIO_17, GPIO_DEV_DEBUG11);
    gpio_dev_unmap(GPIO_18);
    gpio_dev_map(GPIO_18, GPIO_DEV_DEBUG12);
    gpio_dev_unmap(GPIO_19);
    gpio_dev_map(GPIO_19, GPIO_DEV_DEBUG13);
    gpio_dev_unmap(GPIO_20);
    gpio_dev_map(GPIO_20, GPIO_DEV_DEBUG14);
    gpio_dev_unmap(GPIO_21);
    gpio_dev_map(GPIO_21, GPIO_DEV_DEBUG15);
}

void cli_mac802154_diag_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    uint16_t diag_no = 0;
    (void)xWriteBufferLen;

    if ((argc == 2) && (os_strcmp(argv[1], "help") == 0)) {
        cli_mac802154_diag_help();
        msg = CLI_CMD_RSP_SUCCEED;
    } else if ((argc == 4) && (os_strcmp(argv[1], "set") == 0) && (os_strcmp(argv[2], "mac") == 0)) {
        diag_no = os_strtoul(argv[3], NULL, 0) & 0xFFFF;
        dbg_enable_debug_gpio_mac802154();
        sys_hal_diag_debug_mac802154();
        mac802154_diag_debug_send_to_internal(diag_no);
        msg = CLI_CMD_RSP_SUCCEED;
    } else {
        cli_mac802154_diag_help();
        msg = CLI_CMD_RSP_ERROR;
    }

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
#define CLI_CMD_CNT (sizeof(s_mac802154_commands) / sizeof(struct cli_command))
static const struct cli_command s_mac802154_commands[] = {
    {"802154",   "802154 [tx|rx] [channel] [tx_cont|rx_mode]",cli_mac802154},
    {"mac802154_diag", "mac802154_diag set mac [diag_no]", cli_mac802154_diag_cmd},
    {"thread",      "thread dut | thread exit", thread_mac802154_cmd},
    {"txthread",    "txthread -h | -c ch [-l len -n num -f type -t mode -w en -p idx -y duty] | -p idx | -r | -stop", txdtm_mac80154_cmd},
    {"rxthread",    "rxthread -h | -c ch | -g 0 | -r | -stop", rxdtm_mac80154_cmd},
    {"mac802154",   "mac802154 [tx|rx|dut] [enable] [dut_mode] [channel] [dut_tx_channel] [tx_cnt] [pass_cnt_thre]", cli_802154},
};

int cli_mac802154_init(void)
{
    return cli_register_commands(s_mac802154_commands, CLI_CMD_CNT);
}