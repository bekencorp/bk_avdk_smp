//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <soc/soc.h>
#include "driver/int.h"
#include "driver/int_types.h"
#include "sys_driver.h"
#include <ethosu_driver.h>
#include <components/log.h>
#include "modules/pm.h"

#if CONFIG_NPU_CACHE
// Forward declarations to avoid including cache.h which conflicts with CMSIS headers
extern int arch_dcache_flush_range(void *addr, size_t size);
extern int arch_dcache_invd_range(void *addr, size_t size);
#endif

static struct ethosu_driver ethosu0_driver = {0};

extern void bk_delay_us(UINT32 us);

static char TAG[] = "ethosu";
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define BK_ETHOSU_SEMAPHORE_MAX_COUNT 1

#define BK_ETHOSU_SECURE_ENABLE     CONFIG_SPE
#define BK_ETHOSU_PRIVILEGED_ENABLE 1

static uint32_t bk_ethosu_timeout_to_ms(uint64_t timeout)
{
    if (timeout == ETHOSU_SEMAPHORE_WAIT_FOREVER)
    {
        return BEKEN_WAIT_FOREVER;
    }

    if (timeout > UINT32_MAX)
    {
        return BEKEN_WAIT_FOREVER;
    }

    return (uint32_t)timeout;
}

void *ethosu_mutex_create(void)
{
    beken_mutex_t mutex = NULL;

    if (rtos_init_mutex(&mutex) != kNoErr)
    {
        LOGE("Failed to create Ethos-U mutex\r\n");
        return NULL;
    }

    return mutex;
}

void ethosu_mutex_destroy(void *mutex)
{
    beken_mutex_t handle = (beken_mutex_t)mutex;

    if (mutex == NULL)
    {
        return;
    }

    rtos_deinit_mutex(&handle);
}

int ethosu_mutex_lock(void *mutex)
{
    beken_mutex_t handle = (beken_mutex_t)mutex;

    if (mutex == NULL)
    {
        return -1;
    }

    return rtos_lock_mutex(&handle) == kNoErr ? 0 : -1;
}

int ethosu_mutex_unlock(void *mutex)
{
    beken_mutex_t handle = (beken_mutex_t)mutex;

    if (mutex == NULL)
    {
        return -1;
    }

    return rtos_unlock_mutex(&handle) == kNoErr ? 0 : -1;
}

void *ethosu_semaphore_create(void)
{
    beken_semaphore_t semaphore = NULL;

    if (rtos_init_semaphore(&semaphore, BK_ETHOSU_SEMAPHORE_MAX_COUNT) != kNoErr)
    {
        LOGE("Failed to create Ethos-U semaphore\r\n");
        return NULL;
    }

    return semaphore;
}

void ethosu_semaphore_destroy(void *sem)
{
    beken_semaphore_t handle = (beken_semaphore_t)sem;

    if (sem == NULL)
    {
        return;
    }

    rtos_deinit_semaphore(&handle);
}

int ethosu_semaphore_take(void *sem, uint64_t timeout)
{
    beken_semaphore_t handle = (beken_semaphore_t)sem;

    if (sem == NULL)
    {
        return -1;
    }

    return rtos_get_semaphore(&handle, bk_ethosu_timeout_to_ms(timeout)) == kNoErr ? 0 : -1;
}

int ethosu_semaphore_give(void *sem)
{
    beken_semaphore_t handle = (beken_semaphore_t)sem;

    if (sem == NULL)
    {
        return -1;
    }

    return rtos_set_semaphore(&handle) == kNoErr ? 0 : -1;
}

static void bk_ethosu_int_enable(uint32_t enable)
{
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_NPU, enable);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_NPU, enable);
#endif
}

void bk_npu_int_isr(void)
{
    //bk_printf("%s\n", __func__);
    ethosu_irq_handler(&ethosu0_driver);
}

int bk_ethosu_init(void *fast_memory, uint32_t fast_memory_size)
{
    int ret = 0;

    // Disable NPU power down (if needed)
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_NPU, PM_POWER_MODULE_STATE_ON);
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_NPU, PM_CPU_FRQ_480M);

    // Enable NPU clock
    bk_pm_clock_ctrl(PM_CLK_ID_NPU, CLK_PWR_CTRL_PWR_UP);

    // npu memory not enter deep sleep
    //todo use pm api to control npu memory sleep

    // Wait for clock to stabilize
    bk_delay_us(1000);

    // Release NPU nRESET
    sys_drv_set_npu_reset(1);

    // Keep the NPU IRQ masked until the Ethos-U driver handle is fully initialized.
    bk_ethosu_int_enable(0);
    bk_int_isr_register(INT_SRC_NPU, (int_group_isr_t)&bk_npu_int_isr, NULL);

    ret = ethosu_init(&ethosu0_driver,
                      (void *)SOC_NPU_REG_BASE,
                      fast_memory,
                      fast_memory_size,
                      BK_ETHOSU_SECURE_ENABLE,
                      BK_ETHOSU_PRIVILEGED_ENABLE);
    if (ret != 0)
    {
        LOGE("Ethos-U driver init failed, ret=%d\r\n", ret);
        bk_int_isr_unregister(INT_SRC_NPU);
        bk_pm_clock_ctrl(PM_CLK_ID_NPU, CLK_PWR_CTRL_PWR_DOWN);
        bk_pm_module_vote_cpu_freq(PM_DEV_ID_NPU, PM_CPU_FRQ_DEFAULT);
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_NPU, PM_POWER_MODULE_STATE_OFF);
        return ret;
    }

    bk_ethosu_int_enable(1);
    LOGI("Ethos-U driver initialized successfully, ret=%d\r\n", ret);

    return ret;
}

void bk_ethosu_deinit(void)
{
    // Prevent a late NPU IRQ from touching driver resources while they are being freed.
    bk_ethosu_int_enable(0);
    bk_int_isr_unregister(INT_SRC_NPU);

    ethosu_deinit(&ethosu0_driver);
    bk_pm_clock_ctrl(PM_CLK_ID_NPU, CLK_PWR_CTRL_PWR_DOWN);
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_NPU, PM_CPU_FRQ_DEFAULT);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_NPU, PM_POWER_MODULE_STATE_OFF);
}

/**
 * @brief Remap a CPU-visible buffer address to the alias the NPU bus master uses.
 *
 * Overrides the weak identity stub in the Ethos-U core driver. It is invoked by
 * ethosu_dev_run_command_stream() for both the command stream (index -1) and
 * every BASEP region (index 0..n), i.e. every address the NPU dereferences.
 *
 * When CONFIG_SRAM_DIRECT_ADDR is enabled the AP(M55) hands out SRAM buffers in
 * the 0x2Cxxxxxx CPU-direct alias (e.g. the ethos fast scratch from
 * hsram_malloc), but bus masters can only reach SRAM through the 0x28xxxxxx
 * peripheral alias. SOC_SRAM_PERI_ADDR() clears Bit26 for 0x2Cxxxxxx addresses
 * only; PSRAM (0x6xxxxxxx), already-0x28 SRAM and registers pass through
 * unchanged, so it is safe to apply to every address unconditionally.
 */
uint64_t ethosu_address_remap(uint64_t address, int index)
{
    (void)index;
    return (uint64_t)SOC_SRAM_PERI_ADDR((uint32_t)address);
}

#if CONFIG_NPU_CACHE
/**
 * @brief Flush/clean the data cache by address and size
 *
 * This function overrides the weak function in ethosu_driver.c.
 * Only flush specific address range when both p and bytes are valid.
 *
 * @param p     Pointer to the memory address to flush, must be valid
 * @param bytes Size of the memory range to flush, must be > 0
 */
void ethosu_flush_dcache(uint32_t *p, size_t bytes)
{
    // Check parameter validity
    if (p == NULL || bytes == 0)
    {
        return;
    }

    // Flush specific address range
    arch_dcache_flush_range((void *)p, bytes);
}

/**
 * @brief Invalidate the data cache by address and size
 *
 * This function overrides the weak function in ethosu_driver.c.
 * Only invalidate specific address range when both p and bytes are valid.
 *
 * @param p     Pointer to the memory address to invalidate, must be valid
 * @param bytes Size of the memory range to invalidate, must be > 0
 */
void ethosu_invalidate_dcache(uint32_t *p, size_t bytes)
{
    // Check parameter validity
    if (p == NULL || bytes == 0)
    {
        return;
    }

    // Invalidate specific address range
    arch_dcache_invd_range((void *)p, bytes);
}
#endif /* CONFIG_NPU_CACHE */