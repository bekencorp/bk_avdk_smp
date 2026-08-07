#include <os/os.h>
#include <os/mem.h>

#include <components/media_types.h>
#include <components/bk_isp_camera.h>
#include <driver/isp.h>
#include <driver/i2c.h>
#include <driver/io_matrix.h>
#include <modules/pm.h>
#include <avdk_check.h>
#include <components/bk_camera_bus.h>
#include "sw_i2c.h"
#include "isp_camera_ctlr.h"
#include <components/bk_camera_sensor.h>

#define TAG "bk_cam_bus"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define SW_I2C_ENABLE 1

static bk_camera_bus_t *s_bk_camera_bus = NULL;

static int camera_i2c_read_uint8(bk_camera_bus_t *bus, uint8_t reg, uint8_t *value)
{
    i2c_mem_param_t mem_param = {0};

    mem_param.dev_addr = bus->write_address >> 1;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
    mem_param.data_size = bus->data_size;
    mem_param.timeout_ms = bus->timeout_ms;
    mem_param.mem_addr = reg;
    mem_param.data = value;
#if SW_I2C_ENABLE
    return sw_i2c_memory_read(bus->i2c_handle, &mem_param);
#else
    return bk_i2c_memory_read(bus->i2c_id, &mem_param);
#endif
}

static int camera_i2c_read_uint16(bk_camera_bus_t *bus, uint16_t reg, uint8_t *value)
{
    i2c_mem_param_t mem_param = {0};

    mem_param.dev_addr = bus->write_address >> 1;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
    mem_param.data_size = bus->data_size;
    mem_param.timeout_ms = bus->timeout_ms;
    mem_param.mem_addr = reg;
    mem_param.data = value;
#if SW_I2C_ENABLE
    return sw_i2c_memory_read(bus->i2c_handle, &mem_param);
#else
    return bk_i2c_memory_read(bus->i2c_id, &mem_param);
#endif
}

static int camera_i2c_write_uint8(bk_camera_bus_t *bus, uint8_t reg, uint8_t value)
{
    i2c_mem_param_t mem_param = {0};
    mem_param.dev_addr = bus->write_address >> 1;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
    mem_param.data_size = bus->data_size;
    mem_param.timeout_ms = bus->timeout_ms;
    mem_param.mem_addr = reg;
    mem_param.data = (uint8_t *)(&value);

#if SW_I2C_ENABLE
    return sw_i2c_memory_write(bus->i2c_handle, &mem_param);
#else
    return bk_i2c_memory_write(bus->i2c_id, &mem_param);
#endif
}

static int camera_i2c_write_uint16(bk_camera_bus_t *bus, uint16_t reg, uint8_t value)
{
    i2c_mem_param_t mem_param = {0};
    mem_param.dev_addr = bus->write_address >> 1;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
    mem_param.data_size = bus->data_size;
    mem_param.timeout_ms = bus->timeout_ms;
    mem_param.mem_addr = reg;
    mem_param.data = (uint8_t *)(&value);

#if SW_I2C_ENABLE
    return sw_i2c_memory_write(bus->i2c_handle, &mem_param);
#else
    return bk_i2c_memory_write(bus->i2c_id, &mem_param);
#endif
}

bk_camera_bus_t *bk_camera_bus_new(bk_camera_bus_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config, NULL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    bk_camera_bus_t *bus = os_malloc(sizeof(bk_camera_bus_t));
    AVDK_RETURN_ON_FALSE(bus, NULL, TAG, AVDK_ERR_NOMEM_TEXT);

    os_memset(bus, 0, sizeof(bk_camera_bus_t));

    bus->pin_scl = config->pin_scl;
    bus->pin_sda = config->pin_sda;
    bus->i2c_id = config->i2c_id;
    bus->write_address = config->write_address;
    bus->data_size = config->data_size;
    bus->timeout_ms = config->timeout_ms;
    bus->mipi_port_en = config->mipi_port_en;
    bus->dvp_port_en = config->dvp_port_en;

#if SW_I2C_ENABLE
    sw_i2c_config_t i2c_cfg = {0};
    i2c_cfg.sda_pin = bus->pin_sda;
    i2c_cfg.scl_pin = bus->pin_scl;
    bus->i2c_handle = sw_i2c_init(&i2c_cfg);
    // AVDK_RETURN_ON_FALSE(bus->i2c_handle, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
#else
    bk_i2c_driver_init();
    i2c_config_t i2c_cfg = {0};
    i2c_cfg.addr_mode = I2C_ADDR_MODE_7BIT;
    i2c_cfg.baud_rate = I2C_BAUD_RATE_200KHZ;
    bk_i2c_init(bus->i2c_id, &i2c_cfg);
    // AVDK_RETURN_ON_ERROR(bk_i2c_init(bus->i2c_id, &i2c_cfg), TAG, "i2c init fail");
#endif

    bus->read8 = camera_i2c_read_uint8;
    bus->read16 = camera_i2c_read_uint16;
    bus->write8 = camera_i2c_write_uint8;
    bus->write16 = camera_i2c_write_uint16;

    s_bk_camera_bus = bus;
    return bus;
}

avdk_err_t bk_camera_bus_enable(bk_camera_bus_t *bus)
{
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    // isp clk en
    bk_isp_clock_enable(true);

    // auxs/mclk (csi/dvp) clk en
    if (bus->mipi_port_en == 1)
    {
        AVDK_RETURN_ON_ERROR(bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_MIPI_CSI, PM_POWER_MODULE_STATE_ON), TAG, "mipi csi power on failed");
        pm_auxldo_ctrl_cfg_t auxldo_cfg = {0};
        auxldo_cfg.ldo = AUXLDOS_SEL_2P8V;  //csi phy ldo
        auxldo_cfg.out = PM_AUXLDO_2P8V_OUT_2P8V;
        auxldo_cfg.state = PM_AUXLDO_ENABLE;
        auxldo_cfg.user = PM_AUXLDO_USER_CAMERA;
        AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&auxldo_cfg), TAG, "camera 2p8v ldo on failed");

        auxldo_cfg = (pm_auxldo_ctrl_cfg_t){0};
        auxldo_cfg.ldo = AUXLDOS_SEL_3V;  //csi mipi ldo
        auxldo_cfg.out = PM_AUXLDO_3V_OUT_2P8V;
        auxldo_cfg.state = PM_AUXLDO_ENABLE;
        auxldo_cfg.user = PM_AUXLDO_USER_CAMERA;
        AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&auxldo_cfg), TAG, "camera 3v ldo on failed");

        bk_cis_auxs_clock_enable(20000000, 59, 1);
    }
    if (bus->dvp_port_en == 1)
    {
        bk_cis_mclk_clock_enable(20000000, 27, 1);
    }

    return AVDK_ERR_OK;
}

avdk_err_t bk_camera_bus_disable(bk_camera_bus_t *bus)
{
    //AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    if (bus->mipi_port_en == 1)
    {
        bk_cis_auxs_clock_enable(20000000, 59, 0);
        pm_auxldo_ctrl_cfg_t auxldo_cfg = {0};
        auxldo_cfg.ldo = AUXLDOS_SEL_2P8V;  //csi phy ldo
        auxldo_cfg.out = PM_AUXLDO_2P8V_OUT_2P8V;
        auxldo_cfg.state = PM_AUXLDO_DISABLE;
        auxldo_cfg.user = PM_AUXLDO_USER_CAMERA;
        AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&auxldo_cfg), TAG, "camera 2p8v ldo off failed");

        auxldo_cfg = (pm_auxldo_ctrl_cfg_t){0};
        auxldo_cfg.ldo = AUXLDOS_SEL_3V;  //csi mipi ldo
        auxldo_cfg.out = PM_AUXLDO_3V_OUT_2P8V;
        auxldo_cfg.state = PM_AUXLDO_DISABLE;
        auxldo_cfg.user = PM_AUXLDO_USER_CAMERA;
        AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&auxldo_cfg), TAG, "camera 3v ldo off failed");

        AVDK_RETURN_ON_ERROR(bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_MIPI_CSI, PM_POWER_MODULE_STATE_OFF), TAG, "mipi csi power off failed");
    }
    if (bus->dvp_port_en == 1)
    {
        bk_cis_mclk_clock_enable(20000000, 27, 0);
    }

    s_bk_camera_bus = NULL;

#if SW_I2C_ENABLE
    sw_i2c_deinit(bus->i2c_handle);
#else
    bk_i2c_deinit(bus->i2c_id);
#endif

    bk_isp_clock_enable(false);

    return AVDK_ERR_OK;
}

bk_camera_bus_t *bk_camera_bus_get(void)
{
    return s_bk_camera_bus;
}

avdk_err_t bk_camera_bus_delete(bk_camera_bus_t *bus)
{
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    os_free(bus);

    return AVDK_ERR_OK;
}