//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <driver/int.h>
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

    // Enable NPU interrupts for m55a and m55b
    (*(volatile unsigned int*)(0x48000000 + 0x10 * 4)) |= 1<<6;//Enable m55a NPU interrupt
    (*(volatile unsigned int*)(0x48000000 + 0x12 * 4)) |= 1<<7;//Enable m55b NPU interrupt

    // Disable NPU power down (if needed)
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_NPU, PM_POWER_MODULE_STATE_ON);

    // Enable NPU clock
    bk_pm_clock_ctrl(PM_CLK_ID_NPU, CLK_PWR_CTRL_PWR_UP);

    // npu memory not enter deep sleep
    (*(volatile unsigned int*)(0x48000000 + 0x0D * 4)) |= 1<<16;

    // Wait for clock to stabilize
    bk_delay_us(1000);

    // Release NPU nRESET
    (*(volatile unsigned int*)(0x48000000 + 0x06 * 4)) |= 1;//Release NPU nRESET

    // Register NPU interrupt handler
    bk_int_isr_register((icu_int_src_t)6, (int_group_isr_t)&bk_npu_int_isr, NULL);

    ret = ethosu_init(&ethosu0_driver, (void*)0x48200000, fast_memory, fast_memory_size, 1, 1);

    LOGI("Ethos-U driver initialized successfully, ret=%d\r\n", ret);

    return ret;
}

void bk_ethosu_deinit(void)
{
    ethosu_deinit(&ethosu0_driver);

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