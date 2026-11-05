#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <driver/gpio.h>
#include <modules/pm.h>

extern int sdtest_init(void);

int main(void)
{
    bk_err_t ret;

    bk_init();

    ret = bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);
    BK_LOGI("sdtest", "CPU frequency vote 480MHz: ret=%d\r\n", ret);

#if (CONFIG_SDCARD_HOST_ID == 1)
    ret = bk_gpio_enable_output(GPIO_53);
    if (ret == BK_OK)
        ret = bk_gpio_set_output_high(GPIO_53);
    BK_LOGI("sdtest", "SD-NAND power GPIO53 HIGH: ret=%d\r\n", ret);
    rtos_delay_milliseconds(20);
#endif

    ret = sdtest_init();
    BK_LOGI("sdtest", "test CLI register: ret=%d\r\n", ret);

    return 0;
}
