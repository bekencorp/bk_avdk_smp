/*
 * Robot board IMU driver: TDK ICM-42670-P.
 * Register map / WHO_AM_I per DS-000451. Host API mirrors gsensor_sc7a20.c.
 */
#include <string.h>
#include "os/os.h"
#include "os/mem.h"
#include "i2c_hal.h"
#include <driver/i2c.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <components/bk_gsensor.h>
#include <components/log.h>

#define ICM42670P_TAG "icm42670p"
#define ICM42670P_LOGI(...) BK_LOGI(ICM42670P_TAG, ##__VA_ARGS__)
#define ICM42670P_LOGW(...) BK_LOGW(ICM42670P_TAG, ##__VA_ARGS__)
#define ICM42670P_LOGE(...) BK_LOGE(ICM42670P_TAG, ##__VA_ARGS__)
#define ICM42670P_LOGD(...) BK_LOGD(ICM42670P_TAG, ##__VA_ARGS__)

#define GSENSOR_I2C_ID              I2C_ID_1
#define I2C1_SDA_PIN                GPIO_46
#define I2C1_SCL_PIN                GPIO_45
#define GSENSOR_I2C_SDA_PIN         I2C1_SDA_PIN
#define GSENSOR_I2C_SCL_PIN         I2C1_SCL_PIN
#define GSENSOR_G_INT_PIN           GPIO_44

/* ICM-42670-P: AD0 low -> 0x68 */
#define ICM42670P_I2C_ADDR               0x68
#define ICM42670P_WHO_AM_I_VAL           0x67

/* User Bank 0 */
#define ICM42670P_REG_MCLK_RDY           0x00
#define ICM42670P_REG_DEVICE_CONFIG      0x01	//spi mode select
#define ICM42670P_REG_SIGNAL_PATH_RESET  0x02
#define ICM42670P_REG_DEVICE_CONFIG1     0x03	//i3c mode select
#define ICM42670P_REG_DEVICE_CONFIG2     0x04	//i2c mode select
#define ICM42670P_REG_DEVICE_CONFIG3     0x05	//spi mode select
#define ICM42670P_REG_INT_CONFIG         0x06
#define ICM42670P_REG_TEMP_DATA1         0x09
#define ICM42670P_REG_TEMP_DATA0         0x0A

#define ICM42670P_REG_ACCEL_DATA_X1      0x0B
#define ICM42670P_REG_ACCEL_DATA_X0      0x0C
#define ICM42670P_REG_ACCEL_DATA_Y1      0x0D
#define ICM42670P_REG_ACCEL_DATA_Y0      0x0E
#define ICM42670P_REG_ACCEL_DATA_Z1      0x0F
#define ICM42670P_REG_ACCEL_DATA_Z0      0x10

#define ICM42670P_REG_GYRO_DATA_X1       0x11
#define ICM42670P_REG_GYRO_DATA_X0       0x12
#define ICM42670P_REG_GYRO_DATA_Y1       0x13
#define ICM42670P_REG_GYRO_DATA_Y0       0x14
#define ICM42670P_REG_GYRO_DATA_Z1       0x15
#define ICM42670P_REG_GYRO_DATA_Z0       0x16

#define ICM42670P_REG_TMST_FSYNCH        0x17	
#define ICM42670P_REG_TMST_FSYNCL        0x18	

#define ICM42670P_REG_APEX_DATA4         0x1D
#define ICM42670P_REG_APEX_DATA5         0x1E

#define ICM42670P_REG_PWR_MGMT0          0x1F
#define ICM42670P_REG_GYRO_CONFIG0       0x20
#define ICM42670P_REG_ACCEL_CONFIG0      0x21
#define ICM42670P_REG_TEMP_CONFIG0       0x22

#define ICM42670P_REG_GYRO_CONFIG1       0x23
#define ICM42670P_REG_ACCEL_CONFIG1      0x24
#define ICM42670P_REG_APEX_CONFIG0       0x25
#define ICM42670P_REG_APEX_CONFIG1       0x26

#define ICM42670P_REG_WOM_CONFIG         0x27
#define ICM42670P_REG_FIFO_CONFIG1       0x28
#define ICM42670P_REG_FIFO_CONFIG2       0x29
#define ICM42670P_REG_FIFO_CONFIG3       0x2A

#define ICM42670P_REG_INT_SOURCE0        0x2B
#define ICM42670P_REG_INT_SOURCE1        0x2C
#define ICM42670P_REG_INT_SOURCE3        0x2D
#define ICM42670P_REG_INT_SOURCE4        0x2E

#define ICM42670P_REG_FIFO_LOST_PKT0     0x2F
#define ICM42670P_REG_FIFO_LOST_PKT1     0x30

#define ICM42670P_REG_APEX_DATA0         0x31
#define ICM42670P_REG_APEX_DATA1         0x32
#define ICM42670P_REG_APEX_DATA2         0x33
#define ICM42670P_REG_APEX_DATA3         0x34

#define ICM42670P_REG_INIT_CONFIG0       0x35
#define ICM42670P_REG_INIT_CONFIG1       0x36

#define ICM42670P_REG_INT_DRDY           0x39
#define ICM42670P_REG_INT_STATUS         0x3A
#define ICM42670P_REG_INT_STATUS2        0x3B
#define ICM42670P_REG_INT_STATUS3        0x3C

#define ICM42670P_REG_FIFO_COUNTH        0x3D
#define ICM42670P_REG_FIFO_COUNTL        0x3E
#define ICM42670P_REG_FIFO_DATA          0x3F

#define ICM42670P_REG_WHO_AM_I           0x75
#define ICM42670P_REG_BLK_SEL_W          0x79
#define ICM42670P_REG_MADDR_W            0x7A
#define ICM42670P_REG_M_W                0x7B
#define ICM42670P_REG_BLK_SEL_R          0x7C
#define ICM42670P_REG_MADDR_R            0x7D
#define ICM42670P_REG_M_R                0x7E


/* MREG1 */
#define ICM42670P_MREG1_ACCEL_WOM_X_THR  0x4B
#define ICM42670P_MREG1_ACCEL_WOM_Y_THR  0x4C
#define ICM42670P_MREG1_ACCEL_WOM_Z_THR  0x4D


#define ICM42670P_MCLK_RDY_BIT           (1 << 3)
#define ICM42670P_SOFT_RESET_BIT         (1 << 4) /* SIGNAL_PATH_RESET */
#define ICM42670P_PWR_ACCEL_LN           0x03
#define ICM42670P_PWR_ACCEL_LP           0x02
#define ICM42670P_PWR_GYRO_LN            0x0C
#define ICM42670P_PWR_IDLE               0x10

#define ICM42670P_INT1_WOM_XYZ           0x07 /* INT_SOURCE1: WOM X/Y/Z -> INT1 */
#define ICM42670P_WOM_EN                 0x01
#define ICM42670P_WOM_COMPARE_PREV       0x02

#define ICM42670P_POLL_MS                50
#define ICM42670P_WOM_THR                0x20 /* ~125mg @ 1g/256 */

typedef void *gsensor_timer_handle;
typedef void (*gsensor_timer_callback_t)(gsensor_timer_handle timer, void *uarg);

typedef struct {
	beken2_timer_t timer;
	uint8_t type;
	gsensor_timer_callback_t callback;
} app_timer_bk7258_t;

typedef enum {
	GSENSOR_TIMER_TYPE_ONESHOT,
	GSENSOR_TIMER_TYPE_REPEAT,
} GSENSOR_TIMER_TYPE_T;

typedef struct {
	uint32_t i2c_id:2;
	uint32_t device_addr:10;
	uint32_t device_addr_mode:1;
	uint32_t reg_addr:16;
	uint32_t reg_addr_mode:1;
	uint32_t :2;
	uint8_t *data;
	uint32_t data_len;
} i2c_transfer_t;

static beken_semaphore_t s_gsensor_event_wait = NULL;
static beken_thread_t s_gsensor_thread = NULL;
static gsensor_cb datacb = NULL;

static gsensor_mode_t runmode;
static gsensor_dr_t datarate;
static gsensor_range_t datarange;
static uint8_t is_running = 0;
static gsensor_timer_handle gsensor_check_timer;

static void gsensor_check_timer_handler(gsensor_timer_handle timer, void *uarg);
static uint8_t icm42670p_i2c_read(unsigned char reg, unsigned char len, unsigned char *buf);
static uint8_t icm42670p_i2c_write(unsigned char reg, unsigned char data);
static int icm42670p_init(void);
static void icm42670p_deinit(void);
static int icm42670p_open(void);
static void icm42670p_close(void);
static int icm42670p_setDatarate(gsensor_dr_t dr);
static int icm42670p_setMode(gsensor_mode_t mode);
static int icm42670p_setDataRange(gsensor_range_t rg);
static int icm42670p_registerCallback(gsensor_cb cb);

const gsensor_device_t gs_icm42670p = {
	.name = "icm42670p",
	.init = icm42670p_init,
	.deinit = icm42670p_deinit,
	.open = icm42670p_open,
	.close = icm42670p_close,
	.setDatarate = icm42670p_setDatarate,
	.setMode = icm42670p_setMode,
	.setDataRange = icm42670p_setDataRange,
	.registerCallback = icm42670p_registerCallback,
};

static uint8_t icm42670p_odr_from_dr(gsensor_dr_t dr)
{
	switch (dr) {
	case GSENSOR_DR_1HZ:
		return 0x0F; /* 1.5625 Hz, LP */
	case GSENSOR_DR_10HZ:
		return 0x0C; /* 12.5 Hz */
	case GSENSOR_DR_25HZ:
		return 0x0B;
	case GSENSOR_DR_50HZ:
		return 0x0A;
	case GSENSOR_DR_100HZ:
		return 0x09;
	case GSENSOR_DR_200HZ:
		return 0x08;
	case GSENSOR_DR_400HZ:
		return 0x07;
	default:
		return 0x0A;
	}
}

static uint8_t icm42670p_fs_from_range(gsensor_range_t rg)
{
	switch (rg) {
	case GSENSOR_RANGE_2G:
		return (0x3 << 5);
	case GSENSOR_RANGE_4G:
		return (0x2 << 5);
	case GSENSOR_RANGE_8G:
		return (0x1 << 5);
	case GSENSOR_RANGE_16G:
		return (0x0 << 5);
	default:
		return (0x3 << 5);
	}
}

/* Gyro LN ODR codes match accel for >=12.5Hz; 1.5625Hz is accel-LP only. */
static uint8_t icm42670p_gyro_odr_from_dr(gsensor_dr_t dr)
{
	if (dr == GSENSOR_DR_1HZ)
		return 0x0C; /* 12.5 Hz */
	return icm42670p_odr_from_dr(dr);
}

static void gsensor_icm42670p_event_handler(gsensor_mode_t t_runmode)
{
	if (t_runmode == GSENSOR_MODE_NOMAL) {
		unsigned char raw[12];
		gsensor_data_t *dat;

		/* Accel 0x0B..0x10 + Gyro 0x11..0x16, big-endian */
		if (icm42670p_i2c_read(ICM42670P_REG_ACCEL_DATA_X1, 12, raw) != 0)
			return;

		dat = os_malloc(sizeof(gsensor_data_t) + sizeof(gsensor_xyz_t));
		if (dat == NULL) {
			ICM42670P_LOGE("malloc fail\n");
			return;
		}
		dat->count = 1;
		dat->xyz[0].x = (short)((raw[0] << 8) | raw[1]);
		dat->xyz[0].y = (short)((raw[2] << 8) | raw[3]);
		dat->xyz[0].z = (short)((raw[4] << 8) | raw[5]);
		dat->gyro.x = (short)((raw[6] << 8) | raw[7]);
		dat->gyro.y = (short)((raw[8] << 8) | raw[9]);
		dat->gyro.z = (short)((raw[10] << 8) | raw[11]);
		if (datacb)
			datacb((void *)&gs_icm42670p, dat);
		os_free(dat);
	} else if (t_runmode == GSENSOR_MODE_WAKEUP) {
		unsigned char st2 = 0;
		gsensor_data_t nulld;

		/* Clear WOM status by reading INT_STATUS2 */
		(void)icm42670p_i2c_read(ICM42670P_REG_INT_STATUS2, 1, &st2);
		os_memset(&nulld, 0, sizeof(nulld));
		if (datacb)
			datacb((void *)&gs_icm42670p, &nulld);
	}
}

static void gsensor_icm42670p_thread(beken_thread_arg_t arg)
{
	bk_err_t ret;

	(void)arg;
	while (1) {
		ret = rtos_get_semaphore(&s_gsensor_event_wait, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret)
			gsensor_icm42670p_event_handler(runmode);
	}
}

static void gsensor_task_init(void)
{
	uint32_t ret = 0;

	if (s_gsensor_event_wait || s_gsensor_thread)
		return;

	ret = rtos_init_semaphore(&s_gsensor_event_wait, 1);
	if (ret != kNoErr)
		return;

#if CONFIG_PSRAM_AS_SYS_MEMORY
	ret = rtos_create_psram_thread(&s_gsensor_thread, 5, "gsensor_icm42670p",
				       (beken_thread_function_t)gsensor_icm42670p_thread, 1024, NULL);
#else
	ret = rtos_create_thread(&s_gsensor_thread, 5, "gsensor_icm42670p",
				 (beken_thread_function_t)gsensor_icm42670p_thread, 1024, NULL);
#endif
	if (ret != kNoErr) {
		rtos_deinit_semaphore(&s_gsensor_event_wait);
		s_gsensor_event_wait = NULL;
	}
}

static void gsensor_task_deinit(void)
{
	if (s_gsensor_event_wait) {
		rtos_deinit_semaphore(&s_gsensor_event_wait);
		s_gsensor_event_wait = NULL;
	}
	if (s_gsensor_thread) {
		rtos_delete_thread(&s_gsensor_thread);
		s_gsensor_thread = NULL;
	}
}

static void gsensor_check_timer_handler(gsensor_timer_handle timer, void *uarg)
{
	(void)timer;
	(void)uarg;
	rtos_set_semaphore(&s_gsensor_event_wait);
}

static void gsensor_timer_handler(void *Larg, void *Rarg)
{
	app_timer_bk7258_t *app_timer = Larg;

	if (app_timer->type == GSENSOR_TIMER_TYPE_REPEAT)
		rtos_oneshot_reload_timer(&app_timer->timer);
	if (app_timer->callback)
		app_timer->callback(app_timer, Rarg);
}

static void gsensor_timer_create(gsensor_timer_handle *timer, GSENSOR_TIMER_TYPE_T type,
				 unsigned int timeout, gsensor_timer_callback_t callback, void *uarg)
{
	app_timer_bk7258_t *app_timer = os_malloc(sizeof(app_timer_bk7258_t));
	int err;

	os_memset(app_timer, 0, sizeof(app_timer_bk7258_t));
	app_timer->type = type;
	app_timer->callback = callback;
	err = rtos_init_oneshot_timer(&app_timer->timer, timeout, gsensor_timer_handler, app_timer, uarg);
	if (err != 0) {
		ICM42670P_LOGE("%s err %d\r\n", __func__, err);
		os_free(app_timer);
		return;
	}
	*timer = app_timer;
}

static void gsensor_timer_destory(gsensor_timer_handle *timer)
{
	app_timer_bk7258_t *p_app_timer = *timer;

	if (p_app_timer == NULL)
		return;
	if (rtos_is_oneshot_timer_running(&p_app_timer->timer))
		rtos_stop_oneshot_timer(&p_app_timer->timer);
	rtos_deinit_oneshot_timer(&p_app_timer->timer);
	os_free(p_app_timer);
	*timer = NULL;
}

static void gsensor_timer_stop(gsensor_timer_handle timer)
{
	app_timer_bk7258_t *p_app_timer = (app_timer_bk7258_t *)timer;

	if (p_app_timer == NULL)
		return;
	rtos_stop_oneshot_timer(&p_app_timer->timer);
}

static void gsensor_timer_restart(gsensor_timer_handle timer, GSENSOR_TIMER_TYPE_T type,
				  unsigned int timeout, gsensor_timer_callback_t callback, void *uarg)
{
	app_timer_bk7258_t *p_app_timer = (app_timer_bk7258_t *)timer;
	int err;

	if (p_app_timer == NULL)
		return;
	p_app_timer->type = type;
	p_app_timer->callback = callback;
	err = rtos_oneshot_reload_timer_ex(&p_app_timer->timer, timeout, gsensor_timer_handler,
					   p_app_timer, uarg);
	if (err != 0)
		ICM42670P_LOGE("%s err %d\r\n", __func__, err);
}




static void gsensor_gpio_config(gpio_id_t index, gpio_io_mode_t dir, gpio_pull_mode_t pull,
				gpio_func_mode_t peir)
{
	gpio_config_t cfg;

	if (index >= SOC_GPIO_NUM)
		return;
	cfg.io_mode = dir;
	cfg.pull_mode = pull;
	cfg.func_mode = peir;
	bk_gpio_set_config(index, &cfg);
}

static bk_err_t gsensor_i2c_write(i2c_transfer_t *trx)
{
	i2c_mem_param_t mem_param = {0};

	if (trx == NULL)
		return BK_FAIL;
	mem_param.dev_addr = trx->device_addr;
	mem_param.mem_addr = trx->reg_addr;
	mem_param.mem_addr_size = trx->reg_addr_mode;
	mem_param.data = trx->data;
	mem_param.data_size = trx->data_len;
	mem_param.timeout_ms = 2000;
	return bk_i2c_memory_write(trx->i2c_id, &mem_param);
}

static bk_err_t gsensor_i2c_read(i2c_transfer_t *trx)
{
	i2c_mem_param_t mem_param = {0};

	if (trx == NULL)
		return BK_FAIL;
	mem_param.dev_addr = trx->device_addr;
	mem_param.mem_addr = trx->reg_addr;
	mem_param.mem_addr_size = trx->reg_addr_mode;
	mem_param.data = trx->data;
	mem_param.data_size = trx->data_len;
	mem_param.timeout_ms = 2000;
	return bk_i2c_memory_read(trx->i2c_id, &mem_param);
}

static uint8_t icm42670p_i2c_read(unsigned char reg, unsigned char len, unsigned char *buf)
{
	i2c_transfer_t tran = {0};

	tran.i2c_id = GSENSOR_I2C_ID;
	tran.device_addr_mode = 0;
	tran.device_addr = ICM42670P_I2C_ADDR;
	tran.reg_addr_mode = 0;
	tran.reg_addr = reg;
	tran.data = buf;
	tran.data_len = len;
	return (gsensor_i2c_read(&tran) == BK_OK) ? 0 : (uint8_t)-1;
}

static uint8_t icm42670p_i2c_write(unsigned char reg, unsigned char data)
{
	i2c_transfer_t tran = {0};

	tran.i2c_id = GSENSOR_I2C_ID;
	tran.device_addr_mode = 0;
	tran.device_addr = ICM42670P_I2C_ADDR;
	tran.reg_addr_mode = 0;
	tran.reg_addr = reg;
	tran.data = &data;
	tran.data_len = sizeof(data);
	return (gsensor_i2c_write(&tran) == BK_OK) ? 0 : (uint8_t)-1;
}

static int icm42670p_wait_mclk_rdy(void)
{
	unsigned char val = 0;
	int i;

	for (i = 0; i < 50; i++) {
		if (icm42670p_i2c_read(ICM42670P_REG_MCLK_RDY, 1, &val) == 0) {
			if (val & ICM42670P_MCLK_RDY_BIT)
				return 0;
		}
		rtos_delay_milliseconds(1);
	}
	return -1;
}

static int icm42670p_mreg1_write(unsigned char mreg_addr, unsigned char data)
{
	/* Need RC osc when sensors may be off / LP */
	unsigned char pwr = 0;

	if (icm42670p_i2c_read(ICM42670P_REG_PWR_MGMT0, 1, &pwr) != 0)
		return -1;
	(void)icm42670p_i2c_write(ICM42670P_REG_PWR_MGMT0, (unsigned char)(pwr | ICM42670P_PWR_IDLE));
	rtos_delay_milliseconds(1);
	if (icm42670p_wait_mclk_rdy() != 0)
		return -1;

	if (icm42670p_i2c_write(ICM42670P_REG_BLK_SEL_W, 0x00) != 0)
		return -1;
	if (icm42670p_i2c_write(ICM42670P_REG_MADDR_W, mreg_addr) != 0)
		return -1;
	if (icm42670p_i2c_write(ICM42670P_REG_M_W, data) != 0)
		return -1;
	rtos_delay_milliseconds(1);
	(void)icm42670p_i2c_write(ICM42670P_REG_BLK_SEL_W, 0x00);
	return 0;
}

static void icm42670p_apply_accel_config(void)
{
	uint8_t cfg = (uint8_t)(icm42670p_fs_from_range(datarange) | icm42670p_odr_from_dr(datarate));

	(void)icm42670p_i2c_write(ICM42670P_REG_ACCEL_CONFIG0, cfg);
}

/* Default gyro FS = ±2000 dps (GYRO_UI_FS_SEL=0), ODR follows accel rate. */
static void icm42670p_apply_gyro_config(void)
{
	uint8_t cfg = (uint8_t)(0x00 | icm42670p_gyro_odr_from_dr(datarate));

	(void)icm42670p_i2c_write(ICM42670P_REG_GYRO_CONFIG0, cfg);
}

static void gsensor_isr(gpio_id_t gpio_id)
{
	(void)gpio_id;
	gsensor_timer_restart(gsensor_check_timer, GSENSOR_TIMER_TYPE_ONESHOT, 3, gsensor_check_timer_handler, NULL);
}

static int icm42670p_init(void)
{
	i2c_config_t i2c_cfg = {0};
	unsigned char whoami = 0xFF;
	uint8_t ret;

	gsensor_task_init();
	gsensor_gpio_config(GSENSOR_G_INT_PIN, GPIO_INPUT_ENABLE, GPIO_PULL_UP_EN,  GPIO_SECOND_FUNC_DISABLE);
	bk_gpio_register_isr(GSENSOR_G_INT_PIN, gsensor_isr);
	bk_gpio_set_interrupt_type(GSENSOR_G_INT_PIN, GPIO_INT_TYPE_FALLING_EDGE);

	gsensor_timer_create(&gsensor_check_timer, GSENSOR_TIMER_TYPE_REPEAT, ICM42670P_POLL_MS, gsensor_check_timer_handler, NULL);

	i2c_cfg.baud_rate = 100000;
	i2c_cfg.addr_mode = I2C_ADDR_MODE_7BIT;
	i2c_cfg.slave_addr = ICM42670P_I2C_ADDR;
	bk_i2c_init(GSENSOR_I2C_ID, &i2c_cfg);

	/* Soft reset */
	(void)icm42670p_i2c_write(ICM42670P_REG_SIGNAL_PATH_RESET, ICM42670P_SOFT_RESET_BIT);
	rtos_delay_milliseconds(2);

	ret = icm42670p_i2c_read(ICM42670P_REG_WHO_AM_I, 1, &whoami);
	if (ret != 0) {
		ICM42670P_LOGE("read WHO_AM_I fail\r\n");
		return -1;
	}
	ICM42670P_LOGI("ICM-42670-P WHO_AM_I=0x%02x (expect 0x%02x)\r\n", whoami, ICM42670P_WHO_AM_I_VAL);
	if (whoami != ICM42670P_WHO_AM_I_VAL) {
		ICM42670P_LOGE("chip id mismatch\r\n");
		return -1;
	}

	/* Accel off after identify */
	(void)icm42670p_i2c_write(ICM42670P_REG_PWR_MGMT0, 0x00);
	runmode = GSENSOR_MODE_NOMAL;
	datarate = GSENSOR_DR_50HZ;
	datarange = GSENSOR_RANGE_2G;
	is_running = 0;
	return 0;
}

static void icm42670p_deinit(void)
{
	if (is_running) {
		gsensor_timer_stop(gsensor_check_timer);
		bk_gpio_disable_interrupt(GSENSOR_G_INT_PIN);
		is_running = 0;
	}
	gsensor_timer_destory(&gsensor_check_timer);
	(void)icm42670p_i2c_write(ICM42670P_REG_PWR_MGMT0, 0x00);
	bk_i2c_deinit(GSENSOR_I2C_ID);
	gsensor_gpio_config(GSENSOR_G_INT_PIN, GPIO_IO_DISABLE, GPIO_PULL_DISABLE,
			    GPIO_SECOND_FUNC_DISABLE);
	gsensor_task_deinit();
}

static int icm42670p_open(void)
{
	if (is_running)
		return 0;

	rtos_delay_milliseconds(10);

	if (runmode == GSENSOR_MODE_NOMAL) {
		uint8_t pwr;

		icm42670p_apply_accel_config();
		icm42670p_apply_gyro_config();

		/* 1.5625/3.125/6.25 Hz require LP mode per datasheet */
		if (datarate == GSENSOR_DR_1HZ)
			pwr = (uint8_t)(ICM42670P_PWR_GYRO_LN | ICM42670P_PWR_ACCEL_LP);
		else
			pwr = (uint8_t)(ICM42670P_PWR_GYRO_LN | ICM42670P_PWR_ACCEL_LN);
		(void)icm42670p_i2c_write(ICM42670P_REG_PWR_MGMT0, pwr);

		/* Gyro startup ~45ms after mode change (datasheet) */
		rtos_delay_milliseconds(45);
		/* INT1: push-pull, active-low pulsed */
		(void)icm42670p_i2c_write(ICM42670P_REG_INT_CONFIG, 0x02);
		(void)icm42670p_i2c_write(ICM42670P_REG_INT_SOURCE0, 0x08); /* DRDY -> INT1 optional */
		(void)icm42670p_i2c_write(ICM42670P_REG_FIFO_CONFIG1, 0x01); /* bypass */
		gsensor_timer_restart(gsensor_check_timer, GSENSOR_TIMER_TYPE_REPEAT, ICM42670P_POLL_MS,  gsensor_check_timer_handler, NULL);

	} else if (runmode == GSENSOR_MODE_WAKEUP) {
		/* Accel LP + Wake-on-Motion */
		(void)icm42670p_i2c_write(ICM42670P_REG_ACCEL_CONFIG0, (uint8_t)(icm42670p_fs_from_range(datarange) | 0x0C)); /* 12.5Hz */
		(void)icm42670p_i2c_write(ICM42670P_REG_PWR_MGMT0, ICM42670P_PWR_ACCEL_LP);
		rtos_delay_milliseconds(1);

		(void)icm42670p_mreg1_write(ICM42670P_MREG1_ACCEL_WOM_X_THR, ICM42670P_WOM_THR);
		(void)icm42670p_mreg1_write(ICM42670P_MREG1_ACCEL_WOM_Y_THR, ICM42670P_WOM_THR);
		(void)icm42670p_mreg1_write(ICM42670P_MREG1_ACCEL_WOM_Z_THR, ICM42670P_WOM_THR);

		/* Configure WOM before enabling */
		(void)icm42670p_i2c_write(ICM42670P_REG_WOM_CONFIG, ICM42670P_WOM_COMPARE_PREV);
		(void)icm42670p_i2c_write(ICM42670P_REG_INT_CONFIG, 0x02); /* active low */
		(void)icm42670p_i2c_write(ICM42670P_REG_INT_SOURCE0, 0x00);
		(void)icm42670p_i2c_write(ICM42670P_REG_INT_SOURCE1, ICM42670P_INT1_WOM_XYZ);
		(void)icm42670p_i2c_write(ICM42670P_REG_WOM_CONFIG,	 (uint8_t)(ICM42670P_WOM_COMPARE_PREV | ICM42670P_WOM_EN));
		bk_gpio_enable_interrupt(GSENSOR_G_INT_PIN);
	}

	is_running = 1;
	return 0;
}

static void icm42670p_close(void)
{
	if (!is_running)
		return;
	gsensor_timer_stop(gsensor_check_timer);
	bk_gpio_disable_interrupt(GSENSOR_G_INT_PIN);
	(void)icm42670p_i2c_write(ICM42670P_REG_WOM_CONFIG, 0x00);
	(void)icm42670p_i2c_write(ICM42670P_REG_PWR_MGMT0, 0x00);
	is_running = 0;
}

static int icm42670p_setDatarate(gsensor_dr_t dr)
{
	if (dr == datarate)
		return 0;

	datarate = dr;
	if (is_running && runmode == GSENSOR_MODE_NOMAL) {
		uint8_t pwr;

		icm42670p_apply_accel_config();
		icm42670p_apply_gyro_config();
		if (datarate == GSENSOR_DR_1HZ)
			pwr = (uint8_t)(ICM42670P_PWR_GYRO_LN | ICM42670P_PWR_ACCEL_LP);
		else
			pwr = (uint8_t)(ICM42670P_PWR_GYRO_LN | ICM42670P_PWR_ACCEL_LN);
		(void)icm42670p_i2c_write(ICM42670P_REG_PWR_MGMT0, pwr);
	}
	return 0;
}

static int icm42670p_setMode(gsensor_mode_t mode)
{
	if (runmode == mode)
		return 0;

	runmode = mode;
	if (is_running) {
		icm42670p_close();
		rtos_delay_milliseconds(10);
		icm42670p_open();
	}
	return 0;
}

static int icm42670p_setDataRange(gsensor_range_t rg)
{
	if (datarange == rg)
		return 0;
		
	datarange = rg;
	if (is_running && runmode == GSENSOR_MODE_NOMAL)
		icm42670p_apply_accel_config();
	return 0;
}

static int icm42670p_registerCallback(gsensor_cb cb)
{
	datacb = cb;
	return 0;
}
