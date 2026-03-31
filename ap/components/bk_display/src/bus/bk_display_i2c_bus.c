#include <os/os.h>
#include <os/mem.h>
#include <components/bk_display_types.h>
#include <components/bk_display_bus.h>
#include <avdk_check.h>
#include <avdk_error.h>
#include <driver/i2c.h>
#include <sw_i2c.h>
#include <driver/i2c_types.h>

#define TAG "bk_dis_i2c_bus"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct {
    bk_display_bus_ctlr_t ops;
    sw_i2c_handle_t *i2c;
    uint8_t scl_pin;
    uint8_t sda_pin;
} i2c_bus_ctlr_t;

static avdk_err_t bk_display_i2c_bus_delete(bk_display_bus_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    i2c_bus_ctlr_t *bus = __containerof(controller, i2c_bus_ctlr_t, ops);
    if (bus->i2c) {
        sw_i2c_deinit(bus->i2c);
        bus->i2c = NULL;
    }
    os_free(bus);
    return AVDK_ERR_OK;
}

static avdk_err_t bk_display_i2c_bus_read(bk_display_bus_ctlr_t *controller,
                                          bk_display_bus_rw_type_t type,
                                          uint32_t cmd, void *param, size_t size)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(param, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    i2c_bus_ctlr_t *bus = __containerof(controller, i2c_bus_ctlr_t, ops);

    if (type != BK_DISPLAY_BUS_RW_I2C_REG) {
        return AVDK_ERR_UNSUPPORTED;
    }

    uint8_t dev_addr = (uint8_t)((cmd >> 8) & 0xFF);
    uint8_t reg      = (uint8_t)(cmd & 0xFF);

    i2c_mem_param_t mem_param = {0};
    mem_param.dev_addr      = dev_addr;
    mem_param.mem_addr      = reg;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
    mem_param.data          = (uint8_t *)param;
    mem_param.data_size     = (uint32_t)size;
    mem_param.timeout_ms    = 1000;

    bk_err_t ret = sw_i2c_memory_read(bus->i2c, &mem_param);
    return (ret == BK_OK) ? AVDK_ERR_OK : AVDK_ERR_GENERIC;
}

static avdk_err_t bk_display_i2c_bus_write(bk_display_bus_ctlr_t *controller,
                                           bk_display_bus_rw_type_t type,
                                           uint32_t cmd, const void *param, size_t size)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    i2c_bus_ctlr_t *bus = __containerof(controller, i2c_bus_ctlr_t, ops);

    if (type != BK_DISPLAY_BUS_RW_I2C_REG) {
        return AVDK_ERR_UNSUPPORTED;
    }

    uint8_t dev_addr = (uint8_t)((cmd >> 8) & 0xFF);
    uint8_t reg      = (uint8_t)(cmd & 0xFF);

    uint8_t buf[256];
    if (size + 1u > sizeof(buf)) {
        return AVDK_ERR_INVAL;
    }

    buf[0] = reg;
    if (param && size) {
        os_memcpy(&buf[1], param, size);
    }

    bk_err_t ret = sw_i2c_master_write(bus->i2c, dev_addr, buf, 1 + (uint32_t)size, 1000);
    return (ret == BK_OK) ? AVDK_ERR_OK : AVDK_ERR_GENERIC;
}

avdk_err_t bk_display_i2c_bus_new(bk_display_bus_handle_t *handle, const bk_display_i2c_bus_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    i2c_bus_ctlr_t *bus = os_malloc(sizeof(i2c_bus_ctlr_t));
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);

    os_memset(bus, 0, sizeof(i2c_bus_ctlr_t));

    sw_i2c_config_t cfg = {
        .scl_pin = (gpio_id_t)config->scl_pin,
        .sda_pin = (gpio_id_t)config->sda_pin,
    };

    bus->i2c = sw_i2c_init(&cfg);
    if (bus->i2c == NULL) {
        os_free(bus);
        LOGE("sw_i2c_init failed\n");
        return AVDK_ERR_NOMEM;
    }

    bus->scl_pin = config->scl_pin;
    bus->sda_pin = config->sda_pin;

    bus->ops.enable    = NULL;
    bus->ops.disable   = NULL;
    bus->ops.read      = bk_display_i2c_bus_read;
    bus->ops.write     = bk_display_i2c_bus_write;
    bus->ops.delete    = bk_display_i2c_bus_delete;
    bus->ops.set_clock = NULL;
    bus->ops.flush     = NULL;

    *handle = &bus->ops;

    LOGI("new i2c bus scl=%u sda=%u\n", config->scl_pin, config->sda_pin);
    return AVDK_ERR_OK;
}

