#include "isp_i2c_wrapper.h"
#include <components/bk_camera_bus.h>
#include <driver/hal/hal_i2c_types.h>

static beken_mutex_t s_isp_i2c_mutex;

static int isp_i2c_init_wrapper(uint32_t id)
{
    //bk_i2c_init(id);
    return 0;
}

static int isp_i2c_deinit_wrapper(uint32_t id)
{
    //bk_i2c_deinit(id);
    return 0;
}

static int isp_i2c_write_wrapper(uint32_t id, uint16_t slave_addr,
                                 uint32_t addr, uint32_t data,
                                 uint8_t reg_bytes)
{
    (void)id;
    bk_camera_bus_t * bus = bk_camera_bus_get();
    if (bus == NULL)
    {
        return -1;
    }

    if (rtos_lock_mutex(&s_isp_i2c_mutex) != BK_OK)
    {
        return -1;
    }

    /* Keep the shared camera-bus object immutable. The bus callbacks only
     * consume fields from the supplied object, so a transaction-local copy
     * carries the sensor address without racing other camera users. */
    bk_camera_bus_t transaction = *bus;
    transaction.write_address = slave_addr;
    i2c_mem_addr_size_t addr_size = I2C_MEM_ADDR_SIZE_8BIT;
    int ret = -1;

    if (reg_bytes == 2)
    {
        addr_size = I2C_MEM_ADDR_SIZE_16BIT;
    }

    if (addr_size == I2C_MEM_ADDR_SIZE_8BIT)
    {
        ret = transaction.write8(&transaction, addr, data);
    }
    else if (addr_size == I2C_MEM_ADDR_SIZE_16BIT)
    {
        ret = transaction.write16(&transaction, addr, data);
    }

    (void)rtos_unlock_mutex(&s_isp_i2c_mutex);
    return ret;
}

static uint32_t isp_i2c_read_wrapper(uint32_t id, uint16_t slave_addr,
                                     uint32_t addr, uint8_t reg_bytes)
{
    (void)id;
    uint32_t value = 0;
    bk_camera_bus_t * bus = bk_camera_bus_get();
    if (bus == NULL)
    {
        return 0;
    }

    if (rtos_lock_mutex(&s_isp_i2c_mutex) != BK_OK)
    {
        return 0;
    }

    bk_camera_bus_t transaction = *bus;
    transaction.write_address = slave_addr;
    i2c_mem_addr_size_t addr_size = I2C_MEM_ADDR_SIZE_8BIT;

    if (reg_bytes == 2)
    {
        addr_size = I2C_MEM_ADDR_SIZE_16BIT;
    }

    if (addr_size == I2C_MEM_ADDR_SIZE_8BIT)
    {
        transaction.read8(&transaction, addr, (uint8_t *)&value);
    }
    else if (addr_size == I2C_MEM_ADDR_SIZE_16BIT)
    {
        transaction.read16(&transaction, addr, (uint8_t *)&value);
    }

    (void)rtos_unlock_mutex(&s_isp_i2c_mutex);
    return value;
}

static bk_isp_i2c_funcs_t s_isp_i2c_funcs = {
    .i2c_init = isp_i2c_init_wrapper,
    .i2c_deinit = isp_i2c_deinit_wrapper,
    .i2c_write = isp_i2c_write_wrapper,
    .i2c_read = isp_i2c_read_wrapper,
};

extern int vsios_i2c_adapter_init(void *funcs);

bk_err_t bk_isp_i2c_funcs_init(void)
{
    bk_err_t ret = BK_OK;
    if (s_isp_i2c_mutex == NULL &&
        rtos_init_mutex(&s_isp_i2c_mutex) != BK_OK)
    {
        return BK_FAIL;
    }
    if (vsios_i2c_adapter_init(&s_isp_i2c_funcs) != 0)
    {
        ret = BK_FAIL;
    }
    return ret;
}