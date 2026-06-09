/*****************************************************************************
 *
 * General Purpose I2C Bus reference driver (Software I2C Implementation).
 * 
 * REFACTORED: This driver uses a pure handle-based approach. The caller
 * manages the handle pointer, with no internal global state or ID mapping.
 * 
 * Features:
 *  - No per-ID global mapping
 *  - No i2c_id_t parameter (removed for simplicity)
 *  - Caller manages handle lifecycle
 *  - Configurable GPIO pins per instance
 *  - Multiple instances supported (limited only by memory)
 * 
 * Thread Safety:
 *   Software I2C transactions are serialized globally because different
 *   handles may still target the same GPIO pins.
 * 
 *****************************************************************************/

#include <driver/gpio.h>
#include "gpio_driver.h"
#include <os/os.h>
#include <os/mem.h>
#include <common/bk_err.h>
#include <driver/i2c_types.h>
#include <stdbool.h>



#define TRUE 1
#define FALSE 0

#define SUPPORT_100K

/* Disable this macro by default, as gpio api way would cost more time to switch output level */
#define SIM_I2C_GPIO_API_EN

static beken_mutex_t s_sw_i2c_bus_mutex;
static bool s_sw_i2c_bus_mutex_inited;

static bk_err_t sw_i2c_bus_lock(void)
{
	if (!s_sw_i2c_bus_mutex_inited) {
		if (rtos_init_mutex(&s_sw_i2c_bus_mutex) != BK_OK) {
			return BK_FAIL;
		}
		s_sw_i2c_bus_mutex_inited = true;
	}

	rtos_lock_mutex(&s_sw_i2c_bus_mutex);
	return BK_OK;
}

static void sw_i2c_bus_unlock(void)
{
	if (s_sw_i2c_bus_mutex_inited) {
		rtos_unlock_mutex(&s_sw_i2c_bus_mutex);
	}
}

/* Software I2C Handle Structure */
typedef struct {
	gpio_id_t sda_pin;         /**< SDA GPIO pin */
	gpio_id_t scl_pin;         /**< SCL GPIO pin */
} sw_i2c_handle_t;

/* Software I2C Configuration Structure */
typedef struct {
	gpio_id_t sda_pin;         /**< SDA GPIO pin */
	gpio_id_t scl_pin;         /**< SCL GPIO pin */
} sw_i2c_config_t;

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
#define GPIO_OUTPUT_HIGH		0x2
#define GPIO_OUTPUT_LOW			0x0

static inline void i2c_set_sda_input(sw_i2c_handle_t *handle)
{
	bk_gpio_disable_output(handle->sda_pin);
	bk_gpio_enable_input(handle->sda_pin);
}

static inline void i2c_set_sda_output(sw_i2c_handle_t *handle)
{
/* Disable this macro by default, as gpio api way would cost more time to switch output level */
#ifdef SIM_I2C_GPIO_API_EN
	bk_gpio_disable_input(handle->sda_pin);
	bk_gpio_enable_output(handle->sda_pin);
#else
	*((volatile unsigned long *) (SOC_AON_GPIO_REG_BASE+(handle->sda_pin)*4)) = GPIO_OUTPUT_HIGH;
#endif
}

static inline void i2c_set_scl_input(sw_i2c_handle_t *handle) // Is it needed when this is the master? SCL line INPUT?
{
	bk_gpio_disable_output(handle->scl_pin);
	bk_gpio_enable_input(handle->scl_pin);
}

static inline void i2c_set_scl_output(sw_i2c_handle_t *handle)
{
#ifdef SIM_I2C_GPIO_API_EN
	bk_gpio_disable_input(handle->scl_pin);
	bk_gpio_enable_output(handle->scl_pin);
#else
	*((volatile unsigned long *) (SOC_AON_GPIO_REG_BASE+(handle->scl_pin)*4)) = GPIO_OUTPUT_HIGH;
#endif
}

static inline void i2c_set_sda_high(sw_i2c_handle_t *handle)
{
#ifdef SIM_I2C_GPIO_API_EN
	bk_gpio_disable_input(handle->sda_pin);
	bk_gpio_enable_output(handle->sda_pin);
	bk_gpio_set_output_high(handle->sda_pin);
#else
	*((volatile unsigned long *) (SOC_AON_GPIO_REG_BASE+(handle->sda_pin)*4)) = GPIO_OUTPUT_HIGH;
#endif
}

static inline void i2c_set_sda_low(sw_i2c_handle_t *handle)
{
#ifdef SIM_I2C_GPIO_API_EN
	bk_gpio_disable_input(handle->sda_pin);
	bk_gpio_enable_output(handle->sda_pin);
	bk_gpio_set_output_low(handle->sda_pin);
#else
	*((volatile unsigned long *) (SOC_AON_GPIO_REG_BASE+(handle->sda_pin)*4)) = GPIO_OUTPUT_LOW;
#endif
}

static inline void i2c_set_scl_high(sw_i2c_handle_t *handle)
{
#ifdef SIM_I2C_GPIO_API_EN
	bk_gpio_disable_input(handle->scl_pin);
	bk_gpio_enable_output(handle->scl_pin);
	bk_gpio_set_output_high(handle->scl_pin);
#else
	*((volatile unsigned long *) (SOC_AON_GPIO_REG_BASE+(handle->scl_pin)*4)) = GPIO_OUTPUT_HIGH;
#endif
}

static inline void i2c_set_scl_low(sw_i2c_handle_t *handle)
{
#ifdef SIM_I2C_GPIO_API_EN
	bk_gpio_disable_input(handle->scl_pin);
	bk_gpio_enable_output(handle->scl_pin);
	bk_gpio_set_output_low(handle->scl_pin);
#else
	*((volatile unsigned long *) (SOC_AON_GPIO_REG_BASE+(handle->scl_pin)*4)) = GPIO_OUTPUT_LOW;
#endif
}

//This function should return 1 if the SDA is in high state, or 0 if it is in low state
static inline uint8_t i2c_read_sda(sw_i2c_handle_t *handle)
{
	return (uint8_t)(bk_gpio_get_input(handle->sda_pin));
}

void i2c_delay(uint32_t count)
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

static inline void i2c_scl_pulse(sw_i2c_handle_t *handle)
{
	i2c_set_scl_high(handle);
	i2c_delay(SCL_DELAY);

	i2c_set_scl_low(handle);
	i2c_delay(SCL_DELAY);
}

void i2c_scl_pulse_ack(sw_i2c_handle_t *handle)
{
	i2c_set_scl_high(handle);
	i2c_delay(SCL_DELAY);

	i2c_set_scl_low(handle);
	i2c_set_sda_input(handle);
	i2c_delay(SCL_DELAY);
}

void i2c_start(sw_i2c_handle_t *handle)
{
	// Just to ensure SCL is in LOW state.
//	i2c_set_scl_low(handle);
//	i2c_delay(SCL_HALF_DELAY);

	i2c_set_sda_output(handle);
	I2C_PIN_DELAY();

	// SDA SCL in HIGH state
	i2c_set_sda_high(handle);
	I2C_PIN_DELAY();

	i2c_set_scl_high(handle);
	i2c_delay(SCL_DELAY);

	// A HIGH to LOW transition on the SDA line while SCL is HIGH
	i2c_set_sda_low(handle);
	i2c_delay(MAX(SCL_HALF_DELAY, I2C_MIN_DELAY));

	// to default state.
	i2c_set_scl_low(handle);
	i2c_delay(SCL_DELAY);
}

void i2c_stop(sw_i2c_handle_t *handle)
{
	// Just to ensure SCL is in LOW state.
	i2c_set_scl_low(handle);
	i2c_delay(SCL_HALF_DELAY);

	// SDA in LOW state, SCL in HIGH state
	i2c_set_sda_low(handle);
	I2C_PIN_DELAY();

	i2c_set_scl_high(handle);
	i2c_delay(SCL_DELAY);

	// A LOW to HIGH transition on the SDA line while SCL is HIGH
	i2c_set_sda_high(handle);
	i2c_delay(SCL_DELAY);

	// to default state.
	/*
	i2c_set_scl_low(handle);
	i2c_delay(SCL_DELAY);
	*/ // should discard these codes for multi-master I2C bus.( or new devices of latest protocol. )
}

static inline void i2c_ack(sw_i2c_handle_t *handle)
{
	// Pull down SDA line.
	i2c_set_sda_low(handle);
	I2C_PIN_DELAY();

	i2c_scl_pulse(handle);
}

static inline void i2c_no_ack(sw_i2c_handle_t *handle)
{
	// let SDA line in HIGH state during ACK clock cycle.
	i2c_set_sda_high(handle);
	I2C_PIN_DELAY();

	i2c_scl_pulse(handle);
}

void i2c_init(sw_i2c_handle_t *handle)
{
	i2c_set_sda_output(handle);
	i2c_set_scl_output(handle);
	i2c_stop(handle);
}

bool i2c_wait_for_ack(sw_i2c_handle_t *handle)
{
	bool	ack;

	i2c_set_sda_input(handle);
	I2C_PIN_DELAY();

	i2c_set_scl_high(handle);
	i2c_delay(SCL_DELAY);

	ack = (i2c_read_sda(handle) == 0);

	I2C_PIN_DELAY();

	i2c_set_scl_low(handle);
	I2C_PIN_DELAY();

	i2c_delay(SCL_DELAY);

	return ack;
}

bool i2c_send(sw_i2c_handle_t *handle, uint8_t data)
{
	uint8_t	mask;

	i2c_set_sda_output(handle);
	for(mask = 128; mask; mask >>= 1)
	{
		if(data & mask)
		{
			i2c_set_sda_high(handle);
		}
		else
		{
			i2c_set_sda_low(handle);
		}

		I2C_PIN_DELAY();

		if(mask == 1)
		{
			//The 9th bit of ack would exchange the output/input direction
			//Change SDA PIN as input mode in time to improve wave not normative issue
			i2c_scl_pulse_ack(handle);
		} else {
			i2c_scl_pulse(handle);
		}
	}

	return i2c_wait_for_ack(handle);
}

uint8_t i2c_receive(sw_i2c_handle_t *handle, bool last_one)
{
	uint8_t		mask;
	uint8_t		data = 0;

	i2c_set_sda_input(handle);
	I2C_PIN_DELAY();

	for(mask = 128; mask; mask >>= 1)
	{
		i2c_set_scl_high(handle);
		i2c_delay(SCL_DELAY);

		if(i2c_read_sda(handle))
		{
			data |= mask;
		}
		I2C_PIN_DELAY();

		i2c_set_scl_low(handle);
		i2c_delay(SCL_DELAY);
	}

	//Please note - the following call is required because
	//the i2c_set_sda_high and i2c_set_sda_low functions do not return the port to output state
	i2c_set_sda_output(handle);
	I2C_PIN_DELAY();

	if(last_one)
	{
		i2c_no_ack(handle);
	}
	else
	{
		i2c_ack(handle);
	}

	return data;
}

bool i2c_write(sw_i2c_handle_t *handle, uint8_t addr, const uint8_t *buff, uint32_t len)
{
	addr <<= 1;

	i2c_start(handle);

	if(i2c_send(handle, addr) == FALSE)
	{
		i2c_stop(handle);
		return FALSE;
	}

	while(len-- > 0)
	{
		if(i2c_send(handle, *buff++) == FALSE)
		{
			i2c_stop(handle);
			return FALSE;
		}
	}

	i2c_stop(handle);

	return TRUE;
}

bool i2c_read(sw_i2c_handle_t *handle, uint8_t addr, uint8_t *buff, uint32_t len)
{
	if(len == 0)
		return FALSE;

	addr <<= 1;
	addr |= 0x01;

	i2c_start(handle);

	if(i2c_send(handle, addr) == FALSE)
	{
		i2c_stop(handle);
		return FALSE;
	}

	while(len > 0)
	{
		*buff++ = i2c_receive(handle, (--len) == 0);
	}

	i2c_stop(handle);

	return TRUE;
}


sw_i2c_handle_t* sw_i2c_init(const sw_i2c_config_t *cfg)
{
	sw_i2c_handle_t *handle;
	
	if (cfg == NULL)
		return NULL;

	if (sw_i2c_bus_lock() != BK_OK)
		return NULL;
	
	// Allocate handle
	handle = (sw_i2c_handle_t *)os_malloc(sizeof(sw_i2c_handle_t));
	if (handle == NULL)
	{
		sw_i2c_bus_unlock();
		return NULL;
	}
	
	// Set GPIO pins from config
	handle->sda_pin = cfg->sda_pin;
	handle->scl_pin = cfg->scl_pin;

	// Unmap GPIO here as the sw i2c pin is usually dynamical
	gpio_dev_unmap(handle->sda_pin);
	gpio_dev_unmap(handle->scl_pin);

	// Initialize I2C
	i2c_init(handle);

	sw_i2c_bus_unlock();
	return handle;
}

bk_err_t sw_i2c_deinit(sw_i2c_handle_t *handle)
{
	if (handle == NULL)
		return BK_ERR_NULL_PARAM;

	if (sw_i2c_bus_lock() != BK_OK)
		return BK_FAIL;
	
	// Set pins to low before deinit
	i2c_set_scl_low(handle);
	i2c_set_sda_low(handle);

	sw_i2c_bus_unlock();
	
	// Free handle
	os_free(handle);
	
	return BK_OK;
}

static bool i2c_mem_write(sw_i2c_handle_t *handle, uint32_t dev_addr, uint32_t mem_addr, 
                          uint32_t mem_addr_size, const uint8_t *buff, uint32_t len)
{
	uint32_t addr = dev_addr << 1;

	i2c_start(handle);

	if(i2c_send(handle, addr) == FALSE)
	{
		i2c_stop(handle);
		return FALSE;
	}

	if (mem_addr_size == I2C_MEM_ADDR_SIZE_16BIT) {
		if(i2c_send(handle, (mem_addr >> 8) & 0xff) == FALSE)
		{
			i2c_stop(handle);
			return FALSE;
		}
	}

	if(i2c_send(handle, mem_addr & 0xff) == FALSE)
	{
		i2c_stop(handle);
		return FALSE;
	}

	while(len-- > 0)
	{
		if(i2c_send(handle, *buff++) == FALSE)
		{
			i2c_stop(handle);
			return FALSE;
		}
	}

	i2c_stop(handle);

	return TRUE;
}

static bool i2c_mem_read(sw_i2c_handle_t *handle, uint32_t dev_addr, uint32_t mem_addr,
                         uint32_t mem_addr_size, uint8_t *buff, uint32_t len)
{
	uint32_t addr;

	if(len == 0)
		return FALSE;

	addr = dev_addr << 1;

	i2c_start(handle);

	if(i2c_send(handle, addr) == FALSE)
	{
		i2c_stop(handle);
		return FALSE;
	}

	if (mem_addr_size == I2C_MEM_ADDR_SIZE_16BIT) {
		if(i2c_send(handle, (mem_addr >> 8) & 0xff) == FALSE)
		{
			i2c_stop(handle);
			return FALSE;
		}
	}

	if(i2c_send(handle, mem_addr & 0xff) == FALSE)
	{
		i2c_stop(handle);
		return FALSE;
	}

	//////NOTE:  MFI chip need add this stop signal and delay for 30 ms
#ifdef CONFIG_AIRPLAY
	i2c_stop(handle);
	rtos_delay_milliseconds(40);
#endif
	/////////

	i2c_start(handle);
	addr |= 0x01;

	if(i2c_send(handle, addr) == FALSE)
	{
		i2c_stop(handle);
		return FALSE;
	}

	while(len > 0)
	{
		*buff++ = i2c_receive(handle, (--len) == 0);
	}

	i2c_stop(handle);

	return TRUE;
}

bk_err_t sw_i2c_memory_write(sw_i2c_handle_t *handle, i2c_mem_param_t *mem_param)
{
	bk_err_t ret = BK_OK;

	if (handle == NULL || mem_param == NULL || mem_param->data == NULL)
		return BK_ERR_NULL_PARAM;

	ret = sw_i2c_bus_lock();
	if (ret != BK_OK)
		return ret;
	
	if (!i2c_mem_write(handle, mem_param->dev_addr, mem_param->mem_addr, 
	                   mem_param->mem_addr_size, mem_param->data, mem_param->data_size))
		ret = BK_FAIL;
	
	sw_i2c_bus_unlock();
	return ret;
}

bk_err_t sw_i2c_memory_read(sw_i2c_handle_t *handle, i2c_mem_param_t *mem_param)
{
	bk_err_t ret = BK_OK;

	if (handle == NULL || mem_param == NULL || mem_param->data == NULL)
		return BK_ERR_NULL_PARAM;

	ret = sw_i2c_bus_lock();
	if (ret != BK_OK)
		return ret;
	
	if (!i2c_mem_read(handle, mem_param->dev_addr, mem_param->mem_addr,
	                  mem_param->mem_addr_size, mem_param->data, mem_param->data_size))
		ret = BK_FAIL;
	
	sw_i2c_bus_unlock();
	return ret;
}

bk_err_t sw_i2c_master_write(sw_i2c_handle_t *handle, uint32_t dev_addr, const uint8_t *data, uint32_t size, uintptr_t timeout_ms)
{
	if (handle == NULL)
		return BK_ERR_NULL_PARAM;

	if (sw_i2c_bus_lock() != BK_OK)
		return BK_FAIL;

	i2c_write(handle, dev_addr, data, size);
	sw_i2c_bus_unlock();

	return BK_OK;
}

bk_err_t sw_i2c_master_read(sw_i2c_handle_t *handle, uint32_t dev_addr, uint8_t *data, uint32_t size, uintptr_t timeout_ms)
{
	if (handle == NULL)
		return BK_ERR_NULL_PARAM;

	if (sw_i2c_bus_lock() != BK_OK)
		return BK_FAIL;

	i2c_read(handle, dev_addr, data, size);
	sw_i2c_bus_unlock();

	return BK_OK;
}
