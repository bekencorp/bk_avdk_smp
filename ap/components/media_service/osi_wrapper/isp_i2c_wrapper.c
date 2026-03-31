#include "isp_i2c_wrapper.h"
#include <components/bk_camera_bus.h>
#include <driver/hal/hal_i2c_types.h>


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

static int isp_i2c_write_wrapper(uint32_t id, uint32_t addr, uint32_t data, uint8_t reg_bytes)
{
    bk_camera_bus_t * bus = bk_camera_bus_get();
    if (bus == NULL)
    {
        return -1;
    }

    i2c_mem_addr_size_t addr_size = I2C_MEM_ADDR_SIZE_8BIT;

    if (reg_bytes == 2)
    {
        addr_size = I2C_MEM_ADDR_SIZE_16BIT;
    }

    if (addr_size == I2C_MEM_ADDR_SIZE_8BIT)
    {
        return bus->write8(bus, addr, data);
    }
    else if (addr_size == I2C_MEM_ADDR_SIZE_16BIT)
    {
        return bus->write16(bus, addr, data);
    }

    return -1;
}

static uint32_t isp_i2c_read_wrapper(uint32_t id, uint32_t addr, uint8_t reg_bytes)
{
    uint32_t value = 0;
    bk_camera_bus_t * bus = bk_camera_bus_get();
    if (bus == NULL)
    {
        return 0;
    }

    i2c_mem_addr_size_t addr_size = I2C_MEM_ADDR_SIZE_8BIT;

    if (reg_bytes == 2)
    {
        addr_size = I2C_MEM_ADDR_SIZE_16BIT;
    }

    if (addr_size == I2C_MEM_ADDR_SIZE_8BIT)
    {
        bus->read8(bus, addr, (uint8_t *)&value);
    }
    else if (addr_size == I2C_MEM_ADDR_SIZE_16BIT)
    {
        bus->read16(bus, addr, (uint8_t *)&value);
    }

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
    if (vsios_i2c_adapter_init(&s_isp_i2c_funcs) != 0)
    {
        ret = BK_FAIL;
    }
    return ret;
}