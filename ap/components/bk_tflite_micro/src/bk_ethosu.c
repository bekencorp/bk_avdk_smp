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

extern void sys_drv_int_enable_temp(uint32 param);
extern void bk_delay_us(UINT32 us);

static char TAG[] = "ethosu";
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

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

    // Enable NPU clock
    bk_pm_clock_ctrl(PM_CLK_ID_NPU, CLK_PWR_CTRL_PWR_UP);

    // npu memory not enter deep sleep
    //todo use pm api to control npu memory sleep

    // Wait for clock to stabilize
    bk_delay_us(1000);

    // Release NPU nRESET
    sys_drv_set_npu_reset(1);

    // Register NPU interrupt handler
    bk_int_isr_register(INT_SRC_NPU, (int_group_isr_t)&bk_npu_int_isr, NULL);

#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_NPU, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_NPU, 1);
#endif

    ret = ethosu_init(&ethosu0_driver, (void *)SOC_NPU_REG_BASE, fast_memory, fast_memory_size, 1, 1);

    LOGI("Ethos-U driver initialized successfully, ret=%d\r\n", ret);

    return ret;
}

void bk_ethosu_deinit(void)
{
    ethosu_deinit(&ethosu0_driver);
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_NPU, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_NPU, 0);
#endif
    bk_int_isr_unregister(INT_SRC_NPU);

    bk_pm_clock_ctrl(PM_CLK_ID_NPU, CLK_PWR_CTRL_PWR_DOWN);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_NPU, PM_POWER_MODULE_STATE_OFF);
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