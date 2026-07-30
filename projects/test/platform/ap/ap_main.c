#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <stdint.h>
#if CONFIG_BK_NETWORK_PROVISIONING_BLE_EXAMPLE
#include "bk_network_provisioning.h"
#endif
#include "cli.h"
#include "bk_api_ipc_test.h"
#include "dwt.h"
#define APP_TIMEOUT_VALUE    BEKEN_WAIT_FOREVER


#if CONFIG_FREERTOS_SMP
static beken_semaphore_t app_semaphore;

static void cpu1_test_task(void *arg)
{
    BK_LOGD(NULL, "===cpu1_test_task===:\r\n");
    for(;;) {
        rtos_get_semaphore(&app_semaphore, BEKEN_WAIT_FOREVER);
        BK_LOGD(NULL, "cpu1_test_task run core: %d\r\n", rtos_get_core_id());
    }
}

static void cpu2_test_task(void *arg)
{
    BK_LOGD(NULL, "cpu2_test_task run core: %d\r\n", rtos_get_core_id());
  
    for(;;) {
        rtos_set_semaphore(&app_semaphore);
        BK_LOGD(NULL, "cpu2_test_task run core: %d\r\n", rtos_get_core_id());
        rtos_delay_milliseconds(1000);
    }
}

void app_test_smp_core0(void)
{
    int ret;
    beken_thread_t cpu1_thread;

    /* create a semaphore */
    ret = rtos_init_semaphore(&app_semaphore, 5);
    if (ret != kNoErr) {
        BK_LOGD(NULL, "Error: Failed to init app_semaphore: %d\r\n",ret);
    }

    /* create a thread on core 0 */
    ret = rtos_core0_create_thread(&cpu1_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "cpu1_test_task",
                             (beken_thread_function_t)cpu1_test_task,
                             2048,
                             0);
    if (ret != kNoErr) {
        BK_LOGE(NULL, "Error: Failed to create cpu1_test_task: %d\r\n",ret);
    }
}

void app_test_smp_core1(void)
{
    int ret;
    beken_thread_t cpu2_thread;
    
    /* create a thread on core 1 */
    ret = rtos_core1_create_thread(&cpu2_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "cpu2_test_task",
                             (beken_thread_function_t)cpu2_test_task ,
                             2048,
                             0);
    if (ret != kNoErr) {
        BK_LOGE(NULL, "Error: Failed to create cpu2_test_task: %d\r\n",ret);
    }
}
#endif

int32_t bk_sys_uart_write_string(uint32_t uart_id, const char *string);

int main(void)
{
    bk_init();

#if CONFIG_FREERTOS_SMP_TEST
    app_test_smp_core0();
    app_test_smp_core1();
#endif

#if CONFIG_BK_NETWORK_PROVISIONING_BLE_EXAMPLE
extern void demo_network_provisioning_status_cb(bk_network_provisioning_status_t status, void *user_data);
extern void ble_msg_handle_demo_cb(ble_prov_msg_t *msg);
extern int cli_network_provisioning_init(void);
    //for user to receive network provisioning status change event
    bk_register_network_provisioning_status_cb(demo_network_provisioning_status_cb);
    //if default provisioning type is ble, then set msg handle cb
    bk_ble_provisioning_set_msg_handle_cb(ble_msg_handle_demo_cb);
    bk_network_provisioning_init(BK_NETWORK_PROVISIONING_TYPE_BLE);
    cli_network_provisioning_init();
#endif

#if (BK_IPC_UT_TEST)
    bk_ipc_test_init();
#endif

    return 0;
}

static inline void sram_test_clock_init(void)
{
    dwt_init_cycle_counter();
}

static inline uint32_t sram_test_get_cycles(void)
{
    return dwt_get_cycle_counter_val();
}

/* 
#define CONFIG_CP_SRAM_TEST_ADDR             0x2804F700
#define CONFIG_CP_SRAM_TEST_SIZE             0x00010000
#define CONFIG_HIGH_SRAM_TEST_ADDR           0x28110000
#define CONFIG_HIGH_SRAM_TEST_SIZE           0x00010000
#define CONFIG_LOW_SRAM_TEST_ADDR            0x281B0000
#define CONFIG_LOW_SRAM_TEST_SIZE            0x00010000
 */

__attribute__((optimize("O0")))
static void sram_test_write2(uint32_t start_addr, uint32_t size, const char *region)
{
    uint32_t int_level = rtos_disable_int();
    uint32_t start_cycles = sram_test_get_cycles();
    volatile uint32_t *data = (volatile uint32_t *)start_addr;
    uint32_t word_count = size / sizeof(uint32_t);
    
    // Use assembly to write data from CPU0 registers to SRAM using STR
    // Process 1 word (4 bytes) at a time using register r4
    // Write 0xA5A5A5A5 (0xA5 repeated 4 times) to each word
    if (word_count > 0) {
        uint32_t pattern = 0xA5A5A5A5;  // 0xA5 repeated 4 times for 32-bit word
        __asm__ volatile (
            "mov r4, %[pattern]\n"        /* Load pattern 0xA5A5A5A5 into r4 */
            "1:\n"                       /* loop label */
            "str r4, [%[dst]], #4\n"     /* Store 1 word (4 bytes) from r4 to SRAM, increment dst by 4 */
            "subs %[count], %[count], #1\n"  /* Decrement counter */
            "bne 1b\n"                   /* Branch if not zero */
            : [dst] "+r" (data), [count] "+r" (word_count)
            : [pattern] "r" (pattern)
            : "r4", "memory", "cc"
        );
    }
    uint32_t end_cycles = sram_test_get_cycles();
    rtos_enable_int(int_level);
    uint32_t cycles = end_cycles - start_cycles;
    BK_LOGI(NULL, "%s write cycles: %d\r\n", region, cycles);
}

__attribute__((optimize("O0")))
static void sram_test_write(uint32_t start_addr, uint32_t size, const char *region)
{
    uint32_t int_level = rtos_disable_int();
    uint32_t start_cycles = sram_test_get_cycles();
    memset((void *)start_addr, 0, size);
    memset((void *)start_addr, 0xA5, size);
    uint32_t end_cycles = sram_test_get_cycles();
    rtos_enable_int(int_level);
    uint32_t cycles = end_cycles - start_cycles;
    BK_LOGI(NULL, "%s write cycles: %d\r\n", region, cycles);
}

__attribute__((optimize("O0")))
static void sram_test_write_once(uint32_t start_addr, uint32_t size, const char *region)
{
    uint32_t int_level = rtos_disable_int();
    uint32_t start_cycles = sram_test_get_cycles();
    memset((void *)start_addr, 0xA5, size);
    uint32_t end_cycles = sram_test_get_cycles();
    rtos_enable_int(int_level);
    uint32_t cycles = end_cycles - start_cycles;
    BK_LOGI(NULL, "%s write cycles: %d\r\n", region, cycles);
}

__attribute__((optimize("O0")))
static void sram_test_read2(uint32_t start_addr, uint32_t size, const char *region)
{
    uint32_t int_level = rtos_disable_int();
    uint32_t start_cycles = sram_test_get_cycles();
    volatile uint32_t *data = (volatile uint32_t *)start_addr;
    uint32_t word_count = size / sizeof(uint32_t);
    
    // Use assembly to load data into CPU0 registers using LDR
    // Process 1 word (4 bytes) at a time using register r4, only read without storing
    if (word_count > 0) {
        __asm__ volatile (
            "1:\n"                       /* loop label */
            "ldr r4, [%[src]], #4\n"     /* Load 1 word (4 bytes) from SRAM into r4, increment src by 4 */
            "subs %[count], %[count], #1\n"  /* Decrement counter */
            "bne 1b\n"                   /* Branch if not zero */
            : [src] "+r" (data), [count] "+r" (word_count)
            : 
            : "r4", "memory", "cc"
        );
    }
    
    uint32_t end_cycles = sram_test_get_cycles();
    rtos_enable_int(int_level);
    uint32_t cycles = end_cycles - start_cycles;
    BK_LOGI(NULL, "%s read cycles: %d\r\n", region, cycles);
}

static uint32_t *dtcm_data = (uint32_t *)SOC_DTCM_DATA_BASE;

__attribute__((optimize("O0")))
static void sram_test_read(uint32_t start_addr, uint32_t size, const char *region)
{
    uint32_t int_level = rtos_disable_int();
    uint32_t start_cycles = sram_test_get_cycles();
    volatile uint32_t *data = (volatile uint32_t *)start_addr;
    memcpy((void *)dtcm_data, (void *)data, size);
    uint32_t end_cycles = sram_test_get_cycles();
    rtos_enable_int(int_level);
    uint32_t cycles = end_cycles - start_cycles;
    BK_LOGI(NULL, "%s read cycles: %d\r\n", region, cycles);
}

int32_t mem_test(uint32_t address, uint32_t size, uint8_t quiet_mode);
__attribute__((optimize("O0")))
static void sram_mem_test(uint32_t start_addr, uint32_t size, const char *region)
{
    uint32_t int_level = rtos_disable_int();
    uint32_t start_cycles = sram_test_get_cycles();
    mem_test(start_addr, size, 1);
    uint32_t end_cycles = sram_test_get_cycles();
    rtos_enable_int(int_level);
    uint32_t cycles = end_cycles - start_cycles;
    BK_LOGI(NULL, "%s mem test cycles: %d\r\n", region, cycles);
}

static void sram_test_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    BK_LOGI(NULL, "test 28\r\n");
    sram_test_clock_init();
    sram_test_write(CONFIG_CP_SRAM_TEST_ADDR, CONFIG_CP_SRAM_TEST_SIZE, "sram0");
    sram_test_write(CONFIG_HIGH_SRAM_TEST_ADDR, CONFIG_HIGH_SRAM_TEST_SIZE, "sram3");
    sram_test_write(CONFIG_LOW_SRAM_TEST_ADDR, CONFIG_LOW_SRAM_TEST_SIZE, "sram5");
    sram_test_read(CONFIG_CP_SRAM_TEST_ADDR, CONFIG_CP_SRAM_TEST_SIZE, "sram0");
    sram_test_read(CONFIG_HIGH_SRAM_TEST_ADDR, CONFIG_HIGH_SRAM_TEST_SIZE, "sram3");
    sram_test_read(CONFIG_LOW_SRAM_TEST_ADDR, CONFIG_LOW_SRAM_TEST_SIZE, "sram5");
}


static void sram_test_Command2(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    sram_test_clock_init();
    sram_test_write2(CONFIG_CP_SRAM_TEST_ADDR, CONFIG_CP_SRAM_TEST_SIZE, "sram0");
    sram_test_write2(CONFIG_HIGH_SRAM_TEST_ADDR, CONFIG_HIGH_SRAM_TEST_SIZE, "sram3");
    sram_test_write2(CONFIG_LOW_SRAM_TEST_ADDR, CONFIG_LOW_SRAM_TEST_SIZE, "sram5");
    sram_test_read2(CONFIG_CP_SRAM_TEST_ADDR, CONFIG_CP_SRAM_TEST_SIZE, "sram0");
    sram_test_read2(CONFIG_HIGH_SRAM_TEST_ADDR, CONFIG_HIGH_SRAM_TEST_SIZE, "sram3");
    sram_test_read2(CONFIG_LOW_SRAM_TEST_ADDR, CONFIG_LOW_SRAM_TEST_SIZE, "sram5");
}

static void sram_test_Command3(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    BK_LOGI(NULL, "test 28\r\n");
    sram_test_clock_init();
    sram_mem_test(CONFIG_CP_SRAM_TEST_ADDR, CONFIG_CP_SRAM_TEST_SIZE, "sram0");
    sram_mem_test(CONFIG_HIGH_SRAM_TEST_ADDR, CONFIG_HIGH_SRAM_TEST_SIZE, "sram3");
    sram_mem_test(CONFIG_LOW_SRAM_TEST_ADDR, CONFIG_LOW_SRAM_TEST_SIZE, "sram5");
}

static inline uint32_t SwitchAddressTo2C(uint32_t address)
{
    return address + 0x04000000; // 0x04000000 is the offset of 2C
}

static void sram_test_Command4(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    BK_LOGI(NULL, "test 2C\r\n");
    sram_test_clock_init();
    sram_test_write(SwitchAddressTo2C(CONFIG_CP_SRAM_TEST_ADDR), CONFIG_CP_SRAM_TEST_SIZE, "sram0");
    sram_test_write(SwitchAddressTo2C(CONFIG_HIGH_SRAM_TEST_ADDR), CONFIG_HIGH_SRAM_TEST_SIZE, "sram3");
    sram_test_write(SwitchAddressTo2C(CONFIG_LOW_SRAM_TEST_ADDR), CONFIG_LOW_SRAM_TEST_SIZE, "sram5");
    sram_test_read(SwitchAddressTo2C(CONFIG_CP_SRAM_TEST_ADDR), CONFIG_CP_SRAM_TEST_SIZE, "sram0");
    sram_test_read(SwitchAddressTo2C(CONFIG_HIGH_SRAM_TEST_ADDR), CONFIG_HIGH_SRAM_TEST_SIZE, "sram3");
    sram_test_read(SwitchAddressTo2C(CONFIG_LOW_SRAM_TEST_ADDR), CONFIG_LOW_SRAM_TEST_SIZE, "sram5");
}

static void sram_test_Command5(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    BK_LOGI(NULL, "test 2C\r\n");
    sram_test_clock_init();
    sram_mem_test(SwitchAddressTo2C(CONFIG_CP_SRAM_TEST_ADDR), CONFIG_CP_SRAM_TEST_SIZE, "sram0");
    sram_mem_test(SwitchAddressTo2C(CONFIG_HIGH_SRAM_TEST_ADDR), CONFIG_HIGH_SRAM_TEST_SIZE, "sram3");
    sram_mem_test(SwitchAddressTo2C(CONFIG_LOW_SRAM_TEST_ADDR), CONFIG_LOW_SRAM_TEST_SIZE, "sram5");
}

static void sram_test_Command6(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    BK_LOGI(NULL, "test 28, write once\r\n");
    sram_test_clock_init();
    sram_test_write_once(CONFIG_CP_SRAM_TEST_ADDR, CONFIG_CP_SRAM_TEST_SIZE, "sram0");
    sram_test_write_once(CONFIG_HIGH_SRAM_TEST_ADDR, CONFIG_HIGH_SRAM_TEST_SIZE, "sram3");
    sram_test_write_once(CONFIG_LOW_SRAM_TEST_ADDR, CONFIG_LOW_SRAM_TEST_SIZE, "sram5");
    sram_test_read(CONFIG_CP_SRAM_TEST_ADDR, CONFIG_CP_SRAM_TEST_SIZE, "sram0");
    sram_test_read(CONFIG_HIGH_SRAM_TEST_ADDR, CONFIG_HIGH_SRAM_TEST_SIZE, "sram3");
    sram_test_read(CONFIG_LOW_SRAM_TEST_ADDR, CONFIG_LOW_SRAM_TEST_SIZE, "sram5");
}

static void sram_test_Command7(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    BK_LOGI(NULL, "test 2C, write once\r\n");
    sram_test_clock_init();
    sram_test_write_once(SwitchAddressTo2C(CONFIG_CP_SRAM_TEST_ADDR), CONFIG_CP_SRAM_TEST_SIZE, "sram0");
    sram_test_write_once(SwitchAddressTo2C(CONFIG_HIGH_SRAM_TEST_ADDR), CONFIG_HIGH_SRAM_TEST_SIZE, "sram3");
    sram_test_write_once(SwitchAddressTo2C(CONFIG_LOW_SRAM_TEST_ADDR), CONFIG_LOW_SRAM_TEST_SIZE, "sram5");
    sram_test_read(SwitchAddressTo2C(CONFIG_CP_SRAM_TEST_ADDR), CONFIG_CP_SRAM_TEST_SIZE, "sram0");
    sram_test_read(SwitchAddressTo2C(CONFIG_HIGH_SRAM_TEST_ADDR), CONFIG_HIGH_SRAM_TEST_SIZE, "sram3");
    sram_test_read(SwitchAddressTo2C(CONFIG_LOW_SRAM_TEST_ADDR), CONFIG_LOW_SRAM_TEST_SIZE, "sram5");
}

DRV_CLI_CMD_EXPORT static const struct cli_command s_sram_test_commands[] = {
    {"sram_test", "for c", sram_test_Command},
    {"sram_test2", "for asm", sram_test_Command2},
    {"sram_test3", "for mem test", sram_test_Command3},
    {"sram_test4", "for 2C", sram_test_Command4},
    {"sram_test5", "for 2C mem test", sram_test_Command5},
    {"sram_test6", "for 28", sram_test_Command6},
    {"sram_test7", "for 2C", sram_test_Command7},
};

