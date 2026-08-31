/*
 * BK7259 application wrapper around Visionics official VI5302 SDK
 * (VI5302_G_MCU_C01_R00_R14_20260320).
 *
 * Bring-up sequence follows VI5302_main() sample, but:
 *  - does not block forever
 *  - skips factory xtalk/offset calibration by default (needs empty FOV /
 *    known target); apply saved CG_Pos / Offset if set by app
 */
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <driver/gpio.h>
#include <gpio_driver.h>
#include <driver/i2c.h>

#include <bk_tof_vi5302.h>
#include "VI5302_API.h"
#include "VI5302_Firmware.h"
#include "VI5302_System_Data.h"
#include "VI5302_User_Handle.h"

#define VI5302_TAG "vi5302"
#define VI5302_LOGI(...) BK_LOGI(VI5302_TAG, ##__VA_ARGS__)
#define VI5302_LOGW(...) BK_LOGW(VI5302_TAG, ##__VA_ARGS__)
#define VI5302_LOGE(...) BK_LOGE(VI5302_TAG, ##__VA_ARGS__)
#define VI5302_LOGD(...) BK_LOGD(VI5302_TAG, ##__VA_ARGS__)


/* 0 = poll IntrStat register; 1 = GPIO1 falling edge (Kconfig) */

typedef enum {
	TOF_MODE_IDLE = 0,
	TOF_MODE_ONCE,
	TOF_MODE_CONTINUOUS,
} tof_run_mode_t;



static uint8_t s_inited;
static uint8_t s_running;
static volatile tof_run_mode_t s_mode;
static uint32_t s_once_deadline_ms;

static tof_vi5302_cb_t s_cb;
static void *s_cb_arg;

static beken_semaphore_t s_tof_event;
static beken_thread_t s_tof_thread;
static beken_mutex_t s_tof_i2c_mutex;

/*
 * Official SDK passes 8-bit address 0xD8 (incl. R/W=0).
 * Beken bk_i2c_* expects 7-bit address -> 0x6C.
 */
static uint8_t vi5302_dev_addr_7bit(uint8_t addr_8bit)
{
	return (uint8_t)(addr_8bit >> 1);
}

static void tof_i2c_lock(void)
{
	if (s_tof_i2c_mutex)
		(void)rtos_lock_mutex(&s_tof_i2c_mutex);
}

static void tof_i2c_unlock(void)
{
	if (s_tof_i2c_mutex)
		(void)rtos_unlock_mutex(&s_tof_i2c_mutex);
}

#if CONFIG_TOF_VI5302_HW_IRQ
static void tof_hw_irq_crtl(bool flag)
{
	if (flag)
	{
		bk_gpio_enable_interrupt(TOF_INT_PIN);
	}
	else
	{
		bk_gpio_disable_interrupt(TOF_INT_PIN);
	}
}
#endif

static void tof_enter_idle(void)
{
	s_running = 0;
	s_mode = TOF_MODE_IDLE;
	s_once_deadline_ms = 0;
#if CONFIG_TOF_VI5302_HW_IRQ
	tof_hw_irq_crtl(false);
#endif
}



int vi5302_i2c_bus_recover(void)
{
    i2c_config_t cfg = {
        .baud_rate = 400000,
        .addr_mode = I2C_ADDR_MODE_7BIT,
        .slave_addr = vi5302_dev_addr_7bit(VI5302_IIC_DEV_ADDR),
    };

	uint8_t probe = 0;
	bk_i2c_deinit(TOF_I2C_ID);
	// vi5302_i2c_bitbang_recover();
	rtos_delay_milliseconds(2);

	if (bk_i2c_init(TOF_I2C_ID, &cfg) != BK_OK)
		return -1;

	if (VI5302_IIC_Read_One_Byte(REG_INT_STATUS, &probe) != 0)
		return -1;
	return 0;
}


static void tof_gpio_config(gpio_id_t index, gpio_io_mode_t dir, gpio_pull_mode_t pull, gpio_func_mode_t peir)
{
	gpio_config_t cfg;

	if (index >= SOC_GPIO_NUM)
		return;
	cfg.io_mode = dir;
	cfg.pull_mode = pull;
	cfg.func_mode = peir;
	bk_gpio_set_config(index, &cfg);
}

#if CONFIG_TOF_VI5302_HW_IRQ
static void tof_gpio_isr(gpio_id_t id)
{
	(void)id;

	VI5302_GPIO_Interrupt_Handle();
	if(s_tof_event)
		rtos_set_semaphore(&s_tof_event);
}
#endif

static int vi5302_init(void)
{
	i2c_config_t i2c_cfg = {0};  
	i2c_cfg.baud_rate = 400000;
	i2c_cfg.addr_mode = I2C_ADDR_MODE_7BIT;
	i2c_cfg.slave_addr = vi5302_dev_addr_7bit(VI5302_IIC_DEV_ADDR);
	if (bk_i2c_init(TOF_I2C_ID, &i2c_cfg) != BK_OK) 
	{
		VI5302_LOGE("I2C init fail\r\n");
		return -1;
	}

#if CONFIG_TOF_VI5302_HW_IRQ
	//配置 int gpio
	tof_gpio_config(TOF_INT_PIN, GPIO_INPUT_ENABLE, GPIO_PULL_UP_EN,GPIO_SECOND_FUNC_DISABLE);
	bk_gpio_register_isr(TOF_INT_PIN, tof_gpio_isr);
	bk_gpio_set_interrupt_type(TOF_INT_PIN, GPIO_INT_TYPE_FALLING_EDGE);
#endif

	//配置 xshut gpio
	tof_gpio_config(TOF_XSHUT_PIN, GPIO_OUTPUT_ENABLE, GPIO_PULL_UP_EN, GPIO_SECOND_FUNC_DISABLE);
	VI5302_XSHUT_Enable(0);
	VI5302_LOGI("board HAL ready (I2C1, XSHUT=P%d, INT=P%d)\r\n", TOF_XSHUT_PIN, TOF_INT_PIN);
	return 0;
}

static void vi5302_deinit(void)
{
	bk_i2c_deinit(TOF_I2C_ID);
	VI5302_XSHUT_Enable(0);
#if CONFIG_TOF_VI5302_HW_IRQ
	bk_gpio_disable_interrupt(TOF_INT_PIN);
	tof_gpio_config(TOF_INT_PIN, GPIO_IO_DISABLE, GPIO_PULL_DISABLE,GPIO_SECOND_FUNC_DISABLE);
#endif
	tof_gpio_config(TOF_XSHUT_PIN, GPIO_IO_DISABLE, GPIO_PULL_DISABLE,GPIO_SECOND_FUNC_DISABLE);
}

int bk_tof_vi5302_apply_cali(int8_t cg_pos, float offset)
{
	uint8_t ret = 0;

	VI5302_Cali_Data.VI5302_Cali_CG_Pos = cg_pos;
	VI5302_Cali_Data.VI5302_Cali_Offset = offset;
	ret = VI5302_Set_Sys_Xtalk_Position((uint8_t)cg_pos);
	return ret ? -1 : 0;
}


/** Optional one-shot factory calibration (empty FOV + known distance). */
int bk_tof_vi5302_factory_calibrate(uint16_t offset_target_mm)
{
	uint8_t ret = 0;
	uint8_t xtalk_buff[8] = {0};

	if (!s_inited)
		return -1;
  	/******************************** 标定 S ********************************/
    // xtalk标定
	ret |= VI5302_Xtalk_Calibration(xtalk_buff);
	if (ret) {
		VI5302_LOGE("Xtalk cal fail\r\n");
		return -1;
	}
	VI5302_Cali_Data.VI5302_Cali_CG_Pos = (int8_t)xtalk_buff[0];
	// 将xtalk_pos参数写到固件
	ret |= VI5302_Set_Sys_Xtalk_Position((uint8_t)VI5302_Cali_Data.VI5302_Cali_CG_Pos);
	
	// offset标定
	ret |= VI5302_Offset_Calibration(offset_target_mm, &VI5302_Cali_Data.VI5302_Cali_Offset);
	if (ret) {
		VI5302_LOGE("Offset cal fail\r\n");
		return -1;
	}

	VI5302_LOGI("factory cal ok: cg_pos=%d offset=%.2f (save to flash)\r\n",
		    VI5302_Cali_Data.VI5302_Cali_CG_Pos,
		    VI5302_Cali_Data.VI5302_Cali_Offset);
	return 0;
}

int bk_tof_vi5302_get_sp_flag(uint8_t *flag)
{
	if (!flag)
		return -1;
	return VI5302_IIC_Read_One_Byte(REG_SPECIAL_PURP1, flag) ? -1 : 0;
}

int bk_tof_vi5302_heartbeat(uint8_t *val)
{
	if (!val)
		return -1;
	return VI5302_IIC_Read_One_Byte(REG_SPECIAL_PURP2, val) ? -1 : 0;
}


void bk_tof_vi5302_set_calibration(const uint8_t *data, uint32_t len)
{
	(void)data;
	(void)len;
	/*
	 * Official SDK calibration is structured fields (CG_Pos / Offset),
	 * not a raw blob. Use bk_tof_vi5302_apply_cali() instead.
	 */
	VI5302_LOGW("use bk_tof_vi5302_apply_cali(cg_pos, offset) for official SDK\r\n");
}

int bk_tof_vi5302_download_calibration(const uint8_t *data, uint32_t len)
{
	(void)data;
	(void)len;
	return -1;
}

int bk_tof_vi5302_register_callback(tof_vi5302_cb_t cb, void *arg)
{
	s_cb = cb;
	s_cb_arg = arg;
	return 0;
}

static void vi5302_result_to_app(const VI5302_MEASURE_TypeDef *m, tof_vi5302_result_t *out)
{
	out->distance_mm = (uint16_t)(m->correction_tof < 0 ? 0 : m->correction_tof);
	out->confidence = m->confidence;
	out->status = 0;
}

// 轮询线程,用于读取测距数据
static void vi5302_poll_thread(beken_thread_arg_t arg)
{
	(void)arg;
	while (1) 
	{
		uint8_t ret;
		tof_run_mode_t mode;
		VI5302_MEASURE_TypeDef m = {0};
		if (!s_running)
			continue;
#if CONFIG_TOF_VI5302_HW_IRQ
		if (s_tof_event)
			(void)rtos_get_semaphore(&s_tof_event, 40);
#endif

		mode = s_mode;
		tof_i2c_lock();
#if CONFIG_TOF_VI5302_HW_IRQ
		ret = VI5302_Get_Measure_Data(&m, 0);
#else
		ret = VI5302_Get_Measure_Data(&m, 1);
#endif
		if (ret == 0 && mode == TOF_MODE_ONCE)
		{
			tof_enter_idle();
		}
		else if (mode == TOF_MODE_ONCE && s_once_deadline_ms != 0 && rtos_get_time() > s_once_deadline_ms) 
		{
			VI5302_LOGW("once read timeout, idle\r\n");
			tof_enter_idle();
		}
		tof_i2c_unlock();

		if (ret == 0) 
		{
			tof_vi5302_result_t r;
			vi5302_result_to_app(&m, &r);
			VI5302_LOGD("tof=%d mm conf=%u inte=%u\r\n", m.correction_tof, m.confidence, (unsigned)m.intecounts);
			// 回调函数
			if (s_cb)	
			{
				s_cb(s_cb_arg, &r);
			} 
			else if (ret != Result_ERROR) 
			{
				VI5302_LOGW("Get_Measure_Data fail 0x%02x\r\n", ret);
				tof_i2c_lock();
				(void)vi5302_i2c_bus_recover();
				tof_i2c_unlock();
				rtos_delay_milliseconds(50);
			} 
			else
			{
				rtos_delay_milliseconds(1);
			}
		}
	}
}

static int vi5302_task_init(void)
{
	uint32_t ret;

	if (s_tof_thread)
		return 0;

	if (s_tof_i2c_mutex == NULL) {
		ret = rtos_init_mutex(&s_tof_i2c_mutex);
		if (ret != kNoErr)
			return -1;
	}

	ret = rtos_init_semaphore(&s_tof_event, 1);
	if (ret != kNoErr)
		return -1;

#if CONFIG_PSRAM_AS_SYS_MEMORY
	ret = rtos_create_psram_thread(&s_tof_thread, 5, "tof_vi5302", (beken_thread_function_t)vi5302_poll_thread, 4096, NULL);
#else
	ret = rtos_create_thread(&s_tof_thread, 5, "tof_vi5302", (beken_thread_function_t)vi5302_poll_thread, 4096, NULL);
#endif
	if (ret != kNoErr) 
	{
		rtos_deinit_semaphore(&s_tof_event);
		s_tof_event = NULL;
		return -1;
	}
	return 0;
}

static void vi5302_task_deinit(void)
{
	s_running = 0;
	if (s_tof_event)
		rtos_set_semaphore(&s_tof_event);
	if (s_tof_thread) 
	{
		rtos_delete_thread(&s_tof_thread);
		s_tof_thread = NULL;
	}
	if (s_tof_event) 
	{
		rtos_deinit_semaphore(&s_tof_event);
		s_tof_event = NULL;
	}
	if (s_tof_i2c_mutex)
	{
		rtos_deinit_mutex(&s_tof_i2c_mutex);
		s_tof_i2c_mutex = NULL;
	}
	
}

int bk_tof_vi5302_init(void)
{
	uint8_t ret = 0;
	uint8_t fw_ver[16] = {0};
	uint8_t fw_len = 0;

	if (s_inited)
		return 0;

	os_memset(&VI5302_Cali_Data, 0, sizeof(VI5302_Cali_Data));

	// 1、IIC 初始化
	// 2、GPIO 初始化
	if (vi5302_init() != 0)
		return -1;

	//3、选择中断方式：0----寄存器中断，其他值----硬件中断
#if CONFIG_TOF_VI5302_HW_IRQ
	VI5302_Cali_Data.VI5302_Interrupt_Mode_Status = 0x88; /* HW GPIO IRQ */
	VI5302_LOGE("register HW GPIO IRQ mode\r\n");
#else
	VI5302_Cali_Data.VI5302_Interrupt_Mode_Status = 0x00; /* register poll */
	VI5302_LOGE("register SW poll mode\r\n");
#endif

	// 5、VI5302寄存器初始化
	ret |= VI5302_Chip_Register_Init();
	if (ret) {
		VI5302_LOGE("Chip_Register_Init fail 0x%02x\r\n", ret);
		return -1;
	}

	// 6、VI5302固件写入
	/*
	 * Download_Firmware() already checks REG 0x08 == 0x66 once after boot.
	 * Do NOT call Get_VI5302_Download_Firmware_Status() again here: 0x66 is a
	 * one-shot flag cleared before histogram read inside Download_Firmware().
	 */
	ret = VI5302_Download_Firmware((uint8_t *)VI5302_firmware_buff, VI5302_FirmwareSize());
	if (ret) 
	{
		// Firmware 下载标志位检测
		uint8_t fw_st = 0;
		(void)VI5302_IIC_Read_One_Byte(REG_SPECIAL_PURP1, &fw_st);
		VI5302_LOGE("Download_Firmware fail ret=0x%02x REG_0x08=0x%02x (expect 0x66)\r\n",
			    ret, fw_st);
		return -1;
	}
	VI5302_LOGI("firmware download ok, size=%u\r\n", (unsigned)VI5302_FirmwareSize());

	// 默认范围参数来自官方样本
	/* Default ranging params from official sample (30fps, 131072 integrations) */
	ret |= VI5302_Set_Integralcounts_Frame(30, 131072);
	if (ret) {
		VI5302_LOGE("Set_Integralcounts_Frame fail\r\n");
		return -1;
	}

	if (VI5302_Get_FW_Version(fw_ver, &fw_len) == 0 && fw_len)
		VI5302_LOGI("FW ver len=%u first=0x%02x\r\n", fw_len, fw_ver[0]);

	// xtalk标定
	// offset标定
	/*
	 * Factory cal (Xtalk / Offset) should be done once offline, then
	 * restore CG_Pos + Offset here. Skipping runtime cal keeps boot fast
	 * and avoids needing empty FOV / 500mm target on every power-up.
	 */
	VI5302_LOGW("skip runtime xtalk/offset cal; call apply_cali if saved\r\n");

	if (vi5302_task_init() != 0)
		return -1;

	s_inited = 1;
	VI5302_LOGI("VI5302 SDK init ok\r\n");
	return 0;
}

void bk_tof_vi5302_deinit(void)
{
	if (!s_inited)
		return;
	bk_tof_vi5302_stop();
	vi5302_task_deinit();
	vi5302_deinit();
	s_inited = 0;
	s_cb = NULL;
	s_cb_arg = NULL;
}

int bk_tof_vi5302_start_once(void)
{
	uint8_t ret;

	if (!s_inited)
		return -1;

	tof_i2c_lock();
	s_mode = TOF_MODE_ONCE;
	s_running = 1;
	s_once_deadline_ms = rtos_get_time() + 500;
#if CONFIG_TOF_VI5302_HW_IRQ
	tof_hw_irq_crtl(true);
#endif
	ret = VI5302_Start_Single_Ranging_Cmd();
	tof_i2c_unlock();

	if (s_tof_event)
		(void)rtos_set_semaphore(&s_tof_event);
	return ret ? -1 : 0;
}

int bk_tof_vi5302_start_continuous(void)
{
	uint8_t ret;

	if (!s_inited)
		return -1;

	tof_i2c_lock();
	s_mode = TOF_MODE_CONTINUOUS;
	s_running = 1;
	s_once_deadline_ms = 0;
#if CONFIG_TOF_VI5302_HW_IRQ
	tof_hw_irq_crtl(true);
#endif
	ret = VI5302_Start_Continue_Ranging_Cmd();
	tof_i2c_unlock();

	if (s_tof_event)
		(void)rtos_set_semaphore(&s_tof_event);
	return ret ? -1 : 0;
}

int bk_tof_vi5302_stop(void)
{
	uint8_t ret;

	if (!s_inited)
		return -1;

	tof_enter_idle();
	if (s_tof_event)
		(void)rtos_set_semaphore(&s_tof_event);

	tof_i2c_lock();
	ret = VI5302_Stop_Continue_Ranging_Cmd();
	tof_i2c_unlock();
	return ret ? -1 : 0;
}
