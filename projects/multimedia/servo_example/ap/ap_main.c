#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <stdint.h>
#if CONFIG_BK_NETWORK_PROVISIONING_BLE_EXAMPLE
#include "bk_network_provisioning.h"
#endif

#include "bk_api_ipc_test.h"

#if CONFIG_PWM
#include <driver/pwm.h>
#include <driver/pwm_types.h>
#include <driver/gpio.h>
#include <driver/hal/hal_gpio_types.h>
#include "gpio_driver.h"

#define SERVO_CLK_SRC         320000000
#define SERVO_PSC             249
#define SERVO_EFFECTIVE_CLK   (SERVO_CLK_SRC / (SERVO_PSC + 1))
#define SERVO_FREQ            50
#define SERVO_PERIOD_CYCLE    (SERVO_EFFECTIVE_CLK / SERVO_FREQ)
#define SERVO_PULSE_MIN       (SERVO_EFFECTIVE_CLK * 5 / 10000)
#define SERVO_PULSE_MAX       (SERVO_EFFECTIVE_CLK * 25 / 10000)

#define SERVO_CHAN            0

/*
 * Custom GPIO pin configuration for servo PWM output.
 *
 * Uncomment and modify SERVO_USE_CUSTOM_GPIO to override the default
 * GPIO_PWM_MAP_TABLE pin for the servo channel.
 *
 * When defined, after bk_pwm_init() sets up the default GPIO, the code
 * will remap the PWM output to the custom GPIO pin specified here.
 *
 * The custom GPIO must support the corresponding PWM function in its
 * IOMUX configuration. Refer to gpio_map.h for available GPIO-to-PWM
 * mappings on BK7259.
 *
 * Example: use GPIO_6 instead of the default GPIO_47 for PWM channel 0
 */
// #define SERVO_USE_CUSTOM_GPIO
#ifdef SERVO_USE_CUSTOM_GPIO
#define SERVO_CUSTOM_GPIO_ID    GPIO_6
#define SERVO_CUSTOM_GPIO_DEV   GPIO_DEV_PWM0
#endif

static uint32_t servo_angle_to_duty(uint32_t angle)
{
	if (angle > 180)
		angle = 180;
	return SERVO_PULSE_MIN + angle * (SERVO_PULSE_MAX - SERVO_PULSE_MIN) / 180;
}

static void servo_remap_gpio(pwm_chan_t chan)
{
#ifdef SERVO_USE_CUSTOM_GPIO
	(void)chan;
	gpio_dev_unmap(SERVO_CUSTOM_GPIO_ID);
	gpio_dev_map(SERVO_CUSTOM_GPIO_ID, SERVO_CUSTOM_GPIO_DEV);
	bk_gpio_pull_up(SERVO_CUSTOM_GPIO_ID);
	bk_printf("[servo] GPIO remapped: chan=%d -> GPIO_%d (dev=%d)\r\n",
			  chan, SERVO_CUSTOM_GPIO_ID, SERVO_CUSTOM_GPIO_DEV);
#else
	(void)chan;
#endif
}

static void servo_selftest_task(void *arg)
{
	pwm_init_config_t init_cfg = {0};
	pwm_period_duty_config_t duty_cfg = {0};
	pwm_chan_t chan = SERVO_CHAN;

	bk_printf("[servo] self-test start, chan=%d\r\n", chan);

	BK_LOG_ON_ERR(bk_pwm_driver_init());

	init_cfg.period_cycle = SERVO_PERIOD_CYCLE;
	init_cfg.duty_cycle = servo_angle_to_duty(0);
	init_cfg.psc = SERVO_PSC;
	BK_LOG_ON_ERR(bk_pwm_init(chan, &init_cfg));

	servo_remap_gpio(chan);

	BK_LOG_ON_ERR(bk_pwm_start(chan));

	rtos_delay_milliseconds(500);

	duty_cfg.period_cycle = SERVO_PERIOD_CYCLE;
	duty_cfg.psc = SERVO_PSC;

	/* sweep 0 -> 180 -> 0 twice (simulates 360 degree round trip) */
	for (int round = 0; round < 2; round++) {
		bk_printf("[servo] round %d/2: 0 -> 180\r\n", round + 1);
		for (uint32_t angle = 0; angle <= 180; angle += 5) {
			duty_cfg.duty_cycle = servo_angle_to_duty(angle);
			BK_LOG_ON_ERR(bk_pwm_set_period_duty(chan, &duty_cfg));
			rtos_delay_milliseconds(50);
		}

		bk_printf("[servo] round %d/2: 180 -> 0\r\n", round + 1);
		for (int angle = 180; angle >= 0; angle -= 5) {
			duty_cfg.duty_cycle = servo_angle_to_duty((uint32_t)angle);
			BK_LOG_ON_ERR(bk_pwm_set_period_duty(chan, &duty_cfg));
			rtos_delay_milliseconds(50);
		}
	}

	/* return to center position (90 degrees) */
	duty_cfg.duty_cycle = servo_angle_to_duty(90);
	BK_LOG_ON_ERR(bk_pwm_set_period_duty(chan, &duty_cfg));

	bk_printf("[servo] self-test done, parked at 90 degrees\r\n");

	rtos_delete_thread(NULL);
}
#endif

#define APP_TIMEOUT_VALUE    BEKEN_WAIT_FOREVER


#if CONFIG_FREERTOS_SMP
static beken_semaphore_t app_semaphore;

static void smp_test_task1(void *arg)
{
    BK_LOGD(NULL, "===smp_test_task1===:\r\n");
    for(;;) {
        rtos_get_semaphore(&app_semaphore, BEKEN_WAIT_FOREVER);
        BK_LOGD(NULL, "smp_test_task1 run core: %d\r\n", rtos_get_core_id());
    }
}

static void smp_test_task2(void *arg)
{
    BK_LOGD(NULL, "smp_test_task2 run core: %d\r\n", rtos_get_core_id());

    for(;;) {
        rtos_set_semaphore(&app_semaphore);
        BK_LOGD(NULL, "smp_test_task2 run core: %d\r\n", rtos_get_core_id());
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
                             "smp_test_task1",
                             (beken_thread_function_t)smp_test_task1,
                             2048,
                             0);
    if (ret != kNoErr) {
        BK_LOGE(NULL, "Error: Failed to create smp_test_task1: %d\r\n",ret);
    }
}

void app_test_smp_core1(void)
{
    int ret;
    beken_thread_t cpu2_thread;

    /* create a thread on core 1 */
    ret = rtos_core1_create_thread(&cpu2_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "smp_test_task2",
                             (beken_thread_function_t)smp_test_task2 ,
                             2048,
                             0);
    if (ret != kNoErr) {
        BK_LOGE(NULL, "Error: Failed to create smp_test_task2: %d\r\n",ret);
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

#if (BK_IPC_UT_TEST)
    bk_ipc_test_init();
#endif

#if CONFIG_PWM
    {
        beken_thread_t servo_thread;
        int ret = rtos_create_thread(&servo_thread,
                                     BEKEN_DEFAULT_WORKER_PRIORITY,
                                     "servo_test",
                                     (beken_thread_function_t)servo_selftest_task,
                                     4096,
                                     NULL);
        if (ret != kNoErr) {
            bk_printf("[servo] Failed to create self-test thread: %d\r\n", ret);
        }
    }
#endif


    return 0;
}
