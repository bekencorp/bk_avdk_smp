/*****************************************************************************
 *
 * General Purpose software (bit-bang) I2C master driver.
 *
 * Implements a GPIO bit-bang I2C master (START/STOP/ACK, byte send/receive,
 * memory read/write) and exposes it through the id-based bk_i2c_* API:
 *   - Each i2c id maps to a fixed GPIO pin pair (from Kconfig).
 *   - A global bus mutex serializes all transactions (bit-bang shares the GPIO
 *     controller, so a single global lock is required).
 *   - A per-id reference count keeps the bus alive until the last user deinits,
 *     so one module deinit cannot break another module sharing the same id.
 *****************************************************************************/

#include <driver/gpio.h>
#include "gpio_driver.h"
#include <os/os.h>
#include <os/mem.h>
#include <common/bk_err.h>
#include <driver/i2c.h>
#include <driver/i2c_types.h>
#include <components/log.h>
#include <stdbool.h>

#define SIM_I2C_TAG "sim_i2c"
#define SIM_I2C_LOGE(...) BK_LOGE(SIM_I2C_TAG, ##__VA_ARGS__)

#define TRUE 1
#define FALSE 0

#define SUPPORT_100K

/* Software I2C bus context (per instance) */
typedef struct {
	gpio_id_t sda_pin;         /**< SDA GPIO pin */
	gpio_id_t scl_pin;         /**< SCL GPIO pin */
} sim_i2c_bus_t;

/*****************************************************
 * These Macros could be used for RISCV 120Mhz.
 *****************************************************/
#define CLK_DELAY_100K		25
#define CLK_DELAY_400K		1

#define I2C_MIN_DELAY			1

#ifdef SUPPORT_100K
#define SCL_DELAY				CLK_DELAY_100K
#else
#define SCL_DELAY				CLK_DELAY_400K
#endif

#define SCL_HALF_DELAY			((SCL_DELAY + 1) / 2)

/*****************************************************
 * Global bus lock (shared by all ids, since bit-bang
 * shares the GPIO controller).
 *****************************************************/
static beken_mutex_t s_sim_i2c_bus_mutex;
static bool s_sim_i2c_bus_mutex_inited;

static bk_err_t sim_i2c_bus_lock(void)
{
	if (!s_sim_i2c_bus_mutex_inited) {
		if (rtos_init_mutex(&s_sim_i2c_bus_mutex) != BK_OK) {
			return BK_FAIL;
		}
		s_sim_i2c_bus_mutex_inited = true;
	}

	rtos_lock_mutex(&s_sim_i2c_bus_mutex);
	return BK_OK;
}

static void sim_i2c_bus_unlock(void)
{
	if (s_sim_i2c_bus_mutex_inited) {
		rtos_unlock_mutex(&s_sim_i2c_bus_mutex);
	}
}

/*****************************************************
 * Low level GPIO bit-bang primitives.
 *****************************************************/
static inline void i2c_set_sda_input(sim_i2c_bus_t *bus)
{
	bk_gpio_enable_input(bus->sda_pin);
}

static inline void i2c_set_sda_output(sim_i2c_bus_t *bus)
{
	bk_gpio_enable_output(bus->sda_pin);
}

static inline void i2c_set_scl_input(sim_i2c_bus_t *bus) // Is it needed when this is the master? SCL line INPUT?
{
	bk_gpio_enable_input(bus->scl_pin);
}

static inline void i2c_set_scl_output(sim_i2c_bus_t *bus)
{
	bk_gpio_enable_output(bus->scl_pin);
}

static inline void i2c_set_sda_high(sim_i2c_bus_t *bus)
{
	bk_gpio_enable_output(bus->sda_pin);
	bk_gpio_set_output_high(bus->sda_pin);
}

static inline void i2c_set_sda_low(sim_i2c_bus_t *bus)
{
	bk_gpio_enable_output(bus->sda_pin);
	bk_gpio_set_output_low(bus->sda_pin);
}

static inline void i2c_set_scl_high(sim_i2c_bus_t *bus)
{
	bk_gpio_enable_output(bus->scl_pin);
	bk_gpio_set_output_high(bus->scl_pin);
}

static inline void i2c_set_scl_low(sim_i2c_bus_t *bus)
{
	bk_gpio_enable_output(bus->scl_pin);
	bk_gpio_set_output_low(bus->scl_pin);
}

//This function should return 1 if the SDA is in high state, or 0 if it is in low state
static inline uint8_t i2c_read_sda(sim_i2c_bus_t *bus)
{
	return (uint8_t)(bk_gpio_get_input(bus->sda_pin));
}

static void i2c_delay(uint32_t count)
{
	volatile uint32_t 	i;

	for(i = 0; i < count; i++)
	{
	}
}

/************************************************************************************
 *
 * Basic I2C Bus signal.
 *     START, STOP, ACK, No ACK.
 * Pull down SCL line before return from any routines here.
 ************************************************************************************/

#define I2C_PIN_DELAY()		i2c_delay(0)

/* HY4633 touch panel needs >25us between bytes; keep it macro-gated so other
 * devices are unaffected. */
#if CONFIG_TP_HY4633
#define I2C_SPECIAL_DELAY()	i2c_delay(550)
#else
#define I2C_SPECIAL_DELAY()
#endif

static inline void i2c_scl_pulse(sim_i2c_bus_t *bus)
{
	i2c_set_scl_high(bus);
	i2c_delay(SCL_DELAY);

	i2c_set_scl_low(bus);
	i2c_delay(SCL_DELAY);
}

static void i2c_scl_pulse_ack(sim_i2c_bus_t *bus)
{
	i2c_set_scl_high(bus);
	i2c_delay(SCL_DELAY);

	i2c_set_scl_low(bus);
	i2c_set_sda_input(bus);
	i2c_delay(SCL_DELAY);
}

static void i2c_start(sim_i2c_bus_t *bus)
{
	// Just to ensure SCL is in LOW state.
//	i2c_set_scl_low(bus);
//	i2c_delay(SCL_HALF_DELAY);

	i2c_set_sda_output(bus);
	I2C_PIN_DELAY();

	// SDA SCL in HIGH state
	i2c_set_sda_high(bus);
	I2C_PIN_DELAY();

	i2c_set_scl_high(bus);
	i2c_delay(SCL_DELAY);

	// A HIGH to LOW transition on the SDA line while SCL is HIGH
	i2c_set_sda_low(bus);
	i2c_delay(MAX(SCL_HALF_DELAY, I2C_MIN_DELAY));

	// to default state.
	i2c_set_scl_low(bus);
	i2c_delay(SCL_DELAY);
}

static void i2c_stop(sim_i2c_bus_t *bus)
{
	// Just to ensure SCL is in LOW state.
	i2c_set_scl_low(bus);
	i2c_delay(SCL_HALF_DELAY);

	// SDA in LOW state, SCL in HIGH state
	i2c_set_sda_low(bus);
	I2C_PIN_DELAY();

	i2c_set_scl_high(bus);
	i2c_delay(SCL_DELAY);

	// A LOW to HIGH transition on the SDA line while SCL is HIGH
	i2c_set_sda_high(bus);
	i2c_delay(SCL_DELAY);

	// to default state.
	/*
	i2c_set_scl_low(bus);
	i2c_delay(SCL_DELAY);
	*/ // should discard these codes for multi-master I2C bus.( or new devices of latest protocol. )
}

static inline void i2c_ack(sim_i2c_bus_t *bus)
{
	// Pull down SDA line.
	i2c_set_sda_low(bus);
	I2C_PIN_DELAY();

	i2c_scl_pulse(bus);
}

static inline void i2c_no_ack(sim_i2c_bus_t *bus)
{
	// let SDA line in HIGH state during ACK clock cycle.
	i2c_set_sda_high(bus);
	I2C_PIN_DELAY();

	i2c_scl_pulse(bus);
}

/* One-time GPIO setup/validation. Under CONFIG_USR_GPIO_CFG_EN the pins must be
 * declared as GPIO output+input in usr_gpio_cfg.h, otherwise these fail here. */
static bk_err_t sim_i2c_bus_setup(sim_i2c_bus_t *bus)
{
	bk_err_t ret;

	if ((ret = bk_gpio_enable_output(bus->scl_pin)) != BK_OK ||
	    (ret = bk_gpio_enable_input(bus->sda_pin))  != BK_OK ||
	    (ret = bk_gpio_enable_output(bus->sda_pin)) != BK_OK) {
		SIM_I2C_LOGE("gpio setup fail: scl=%d sda=%d ret=%d (check usr_gpio_cfg.h)\r\n",
		             bus->scl_pin, bus->sda_pin, ret);
		return ret;
	}

	return BK_OK;
}

static bk_err_t i2c_init(sim_i2c_bus_t *bus)
{
	bk_err_t ret = sim_i2c_bus_setup(bus);
	if (ret != BK_OK)
		return ret;

	i2c_stop(bus);
	return BK_OK;
}

static bool i2c_wait_for_ack(sim_i2c_bus_t *bus)
{
	bool	ack;

	i2c_set_sda_input(bus);
	I2C_PIN_DELAY();

	i2c_set_scl_high(bus);
	i2c_delay(SCL_DELAY);

	ack = (i2c_read_sda(bus) == 0);

	I2C_PIN_DELAY();

	i2c_set_scl_low(bus);
	I2C_PIN_DELAY();

	i2c_delay(SCL_DELAY);

	I2C_SPECIAL_DELAY();

	return ack;
}

static bool i2c_send(sim_i2c_bus_t *bus, uint8_t data)
{
	uint8_t	mask;

	i2c_set_sda_output(bus);
	for(mask = 128; mask; mask >>= 1)
	{
		if(data & mask)
		{
			i2c_set_sda_high(bus);
		}
		else
		{
			i2c_set_sda_low(bus);
		}

		I2C_PIN_DELAY();

		if(mask == 1)
		{
			//The 9th bit of ack would exchange the output/input direction
			//Change SDA PIN as input mode in time to improve wave not normative issue
			i2c_scl_pulse_ack(bus);
		} else {
			i2c_scl_pulse(bus);
		}
	}

	return i2c_wait_for_ack(bus);
}

static uint8_t i2c_receive(sim_i2c_bus_t *bus, bool last_one)
{
	uint8_t		mask;
	uint8_t		data = 0;

	i2c_set_sda_input(bus);
	I2C_PIN_DELAY();

	for(mask = 128; mask; mask >>= 1)
	{
		i2c_set_scl_high(bus);
		i2c_delay(SCL_DELAY);

		if(i2c_read_sda(bus))
		{
			data |= mask;
		}
		I2C_PIN_DELAY();

		i2c_set_scl_low(bus);
		i2c_delay(SCL_DELAY);
	}

	//Please note - the following call is required because
	//the i2c_set_sda_high and i2c_set_sda_low functions do not return the port to output state
	i2c_set_sda_output(bus);
	I2C_PIN_DELAY();

	if(last_one)
	{
		i2c_no_ack(bus);
	}
	else
	{
		i2c_ack(bus);
	}

	I2C_SPECIAL_DELAY();

	return data;
}

static bool i2c_write(sim_i2c_bus_t *bus, uint8_t addr, const uint8_t *buff, uint32_t len)
{
	addr <<= 1;

	i2c_start(bus);

	if(i2c_send(bus, addr) == FALSE)
	{
		i2c_stop(bus);
		return FALSE;
	}

	while(len-- > 0)
	{
		if(i2c_send(bus, *buff++) == FALSE)
		{
			i2c_stop(bus);
			return FALSE;
		}
	}

	i2c_stop(bus);

	return TRUE;
}

static bool i2c_read(sim_i2c_bus_t *bus, uint8_t addr, uint8_t *buff, uint32_t len)
{
	if(len == 0)
		return FALSE;

	addr <<= 1;
	addr |= 0x01;

	i2c_start(bus);

	if(i2c_send(bus, addr) == FALSE)
	{
		i2c_stop(bus);
		return FALSE;
	}

	while(len > 0)
	{
		*buff++ = i2c_receive(bus, (--len) == 0);
	}

	i2c_stop(bus);

	return TRUE;
}

static bool i2c_mem_write(sim_i2c_bus_t *bus, uint32_t dev_addr, uint32_t mem_addr,
                          uint32_t mem_addr_size, const uint8_t *buff, uint32_t len)
{
	uint32_t addr = dev_addr << 1;

	i2c_start(bus);

	if(i2c_send(bus, addr) == FALSE)
	{
		i2c_stop(bus);
		return FALSE;
	}

	if (mem_addr_size == I2C_MEM_ADDR_SIZE_16BIT) {
		if(i2c_send(bus, (mem_addr >> 8) & 0xff) == FALSE)
		{
			i2c_stop(bus);
			return FALSE;
		}
	}

	if(i2c_send(bus, mem_addr & 0xff) == FALSE)
	{
		i2c_stop(bus);
		return FALSE;
	}

	while(len-- > 0)
	{
		if(i2c_send(bus, *buff++) == FALSE)
		{
			i2c_stop(bus);
			return FALSE;
		}
	}

	i2c_stop(bus);

	return TRUE;
}

static bool i2c_mem_read(sim_i2c_bus_t *bus, uint32_t dev_addr, uint32_t mem_addr,
                         uint32_t mem_addr_size, uint8_t *buff, uint32_t len)
{
	uint32_t addr;

	if(len == 0)
		return FALSE;

	addr = dev_addr << 1;

	i2c_start(bus);

	if(i2c_send(bus, addr) == FALSE)
	{
		i2c_stop(bus);
		return FALSE;
	}

	if (mem_addr_size == I2C_MEM_ADDR_SIZE_16BIT) {
		if(i2c_send(bus, (mem_addr >> 8) & 0xff) == FALSE)
		{
			i2c_stop(bus);
			return FALSE;
		}
	}

	if(i2c_send(bus, mem_addr & 0xff) == FALSE)
	{
		i2c_stop(bus);
		return FALSE;
	}

	//////NOTE:  MFI chip need add this stop signal and delay for 30 ms
#ifdef CONFIG_AIRPLAY
	i2c_stop(bus);
	rtos_delay_milliseconds(40);
#endif
	/////////

	i2c_start(bus);
	addr |= 0x01;

	if(i2c_send(bus, addr) == FALSE)
	{
		i2c_stop(bus);
		return FALSE;
	}

	while(len > 0)
	{
		*buff++ = i2c_receive(bus, (--len) == 0);
	}

	i2c_stop(bus);

	return TRUE;
}

/*****************************************************
 * id-based facade on top of the bit-bang engine.
 *****************************************************/
static const sim_i2c_bus_t s_sim_pin[I2C_ID_MAX] = {
	[I2C_ID_0] = { CONFIG_SIM_I2C0_SDA_GPIO, CONFIG_SIM_I2C0_SCL_GPIO },
#if (SOC_I2C_UNIT_NUM > 1)
	[I2C_ID_1] = { CONFIG_SIM_I2C1_SDA_GPIO, CONFIG_SIM_I2C1_SCL_GPIO },
#endif
};

static sim_i2c_bus_t s_bus[I2C_ID_MAX];
static uint32_t s_ref[I2C_ID_MAX];

bk_err_t bk_i2c_driver_init(void)
{
	if (!s_sim_i2c_bus_mutex_inited) {
		if (rtos_init_mutex(&s_sim_i2c_bus_mutex) != BK_OK) {
			return BK_FAIL;
		}
		s_sim_i2c_bus_mutex_inited = true;
	}

	return BK_OK;
}

bk_err_t bk_i2c_driver_deinit(void)
{
	return BK_OK;
}

bk_err_t bk_i2c_init(i2c_id_t id, const i2c_config_t *cfg)
{
	if (id >= I2C_ID_MAX)
		return BK_ERR_I2C_ID_NOT_INIT;

	uint32_t int_level = rtos_enter_critical();
	bool need_hw_init = (s_ref[id] == 0);
	s_ref[id]++;
	if (need_hw_init)
		s_bus[id] = s_sim_pin[id];
	rtos_exit_critical(int_level);

	if (need_hw_init) {
		bk_err_t ret = i2c_init(&s_bus[id]);
		if (ret != BK_OK) {
			uint32_t lvl = rtos_enter_critical();
			if (s_ref[id] > 0)
				s_ref[id]--;
			rtos_exit_critical(lvl);
			return ret;
		}
	}

	return BK_OK;
}

bk_err_t bk_i2c_deinit(i2c_id_t id)
{
	if (id >= I2C_ID_MAX)
		return BK_ERR_I2C_ID_NOT_INIT;

	uint32_t int_level = rtos_enter_critical();
	bool do_release = false;
	if (s_ref[id] > 0) {
		s_ref[id]--;
		do_release = (s_ref[id] == 0);
	}
	rtos_exit_critical(int_level);

	if (do_release) {
		i2c_set_scl_low(&s_bus[id]);
		i2c_set_sda_low(&s_bus[id]);
	}

	return BK_OK;
}

bk_err_t bk_i2c_memory_write(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
	bk_err_t ret;

	if (id >= I2C_ID_MAX || s_ref[id] == 0)
		return BK_ERR_I2C_ID_NOT_INIT;
	if (mem_param == NULL || mem_param->data == NULL)
		return BK_ERR_NULL_PARAM;

	ret = sim_i2c_bus_lock();
	if (ret != BK_OK)
		return ret;

	if (!i2c_mem_write(&s_bus[id], mem_param->dev_addr, mem_param->mem_addr,
	                   mem_param->mem_addr_size, mem_param->data, mem_param->data_size))
		ret = BK_FAIL;

	sim_i2c_bus_unlock();
	return ret;
}

bk_err_t bk_i2c_memory_read(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
	bk_err_t ret;

	if (id >= I2C_ID_MAX || s_ref[id] == 0)
		return BK_ERR_I2C_ID_NOT_INIT;
	if (mem_param == NULL || mem_param->data == NULL)
		return BK_ERR_NULL_PARAM;

	ret = sim_i2c_bus_lock();
	if (ret != BK_OK)
		return ret;

	if (!i2c_mem_read(&s_bus[id], mem_param->dev_addr, mem_param->mem_addr,
	                  mem_param->mem_addr_size, mem_param->data, mem_param->data_size))
		ret = BK_FAIL;

	sim_i2c_bus_unlock();
	return ret;
}

bk_err_t bk_i2c_master_write(i2c_id_t id, uint32_t dev_addr, const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	bk_err_t ret;

	if (id >= I2C_ID_MAX || s_ref[id] == 0)
		return BK_ERR_I2C_ID_NOT_INIT;

	ret = sim_i2c_bus_lock();
	if (ret != BK_OK)
		return ret;

	if (!i2c_write(&s_bus[id], dev_addr, data, size))
		ret = BK_FAIL;

	sim_i2c_bus_unlock();
	return ret;
}

bk_err_t bk_i2c_master_read(i2c_id_t id, uint32_t dev_addr, uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	bk_err_t ret;

	if (id >= I2C_ID_MAX || s_ref[id] == 0)
		return BK_ERR_I2C_ID_NOT_INIT;

	ret = sim_i2c_bus_lock();
	if (ret != BK_OK)
		return ret;

	if (!i2c_read(&s_bus[id], dev_addr, data, size))
		ret = BK_FAIL;

	sim_i2c_bus_unlock();
	return ret;
}
