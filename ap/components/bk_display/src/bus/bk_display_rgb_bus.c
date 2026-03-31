#include <os/os.h>
#include <os/mem.h>
#include <components/bk_display_bus.h>
#include <avdk_check.h>
#include "display_rgb_bus_vn_ctlr.h"
#include <driver/gpio.h>
#include "gpio_driver.h"
#define TAG "bk_rgb_bus"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define IO_FUNCTION_ENABLE(pin, func)   \
    do {                                \
        gpio_dev_unmap(pin);            \
        gpio_dev_map(pin, func);        \
        bk_gpio_set_capacity(pin, GPIO_DRIVER_CAPACITY_3);  \
    } while (0)

#define LCD_SPI_DELAY     2

extern void delay(INT32 num);

static bk_err_t rgb_gpio_init(void)
{
    IO_FUNCTION_ENABLE(LCD_RGB_R0_PIN, LCD_RGB_R0_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_R1_PIN, LCD_RGB_R1_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_R2_PIN, LCD_RGB_R2_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_R3_PIN, LCD_RGB_R3_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_R4_PIN, LCD_RGB_R4_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_R5_PIN, LCD_RGB_R5_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_R6_PIN, LCD_RGB_R6_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_R7_PIN, LCD_RGB_R7_FUNC);

    IO_FUNCTION_ENABLE(LCD_RGB_G0_PIN, LCD_RGB_G0_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_G1_PIN, LCD_RGB_G1_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_G2_PIN, LCD_RGB_G2_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_G3_PIN, LCD_RGB_G3_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_G4_PIN, LCD_RGB_G4_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_G5_PIN, LCD_RGB_G5_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_G6_PIN, LCD_RGB_G6_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_G7_PIN, LCD_RGB_G7_FUNC);

    IO_FUNCTION_ENABLE(LCD_RGB_B0_PIN, LCD_RGB_B0_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_B1_PIN, LCD_RGB_B1_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_B2_PIN, LCD_RGB_B2_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_B3_PIN, LCD_RGB_B3_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_B4_PIN, LCD_RGB_B4_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_B5_PIN, LCD_RGB_B5_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_B6_PIN, LCD_RGB_B6_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_B7_PIN, LCD_RGB_B7_FUNC);

    IO_FUNCTION_ENABLE(LCD_RGB_CLK_PIN, LCD_RGB_CLK_FUNC);

    gpio_dev_unmap(LCD_RGB_DISP_PIN);
    BK_LOG_ON_ERR(bk_gpio_enable_output(LCD_RGB_DISP_PIN));
    bk_gpio_set_output_high(LCD_RGB_DISP_PIN);

    IO_FUNCTION_ENABLE(LCD_RGB_HSYNC_PIN, LCD_RGB_HSYNC_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_VSYNC_PIN, LCD_RGB_VSYNC_FUNC);
    IO_FUNCTION_ENABLE(LCD_RGB_DE_PIN, LCD_RGB_DE_FUNC);
    LOGI("%s, complete\n", __func__);  
    return BK_OK;
}

void rgb_bus_spi_gpio_init(uint8_t sda, uint8_t clk, uint8_t csx, uint8_t rst)
{
    LOGI("%s, sda: %d, clk: %d, csx: %d, rst: %d\n", __func__, sda, clk, csx, rst);

    gpio_dev_unmap(rst);
    bk_gpio_set_capacity(rst, GPIO_DRIVER_CAPACITY_3);
    BK_LOG_ON_ERR(bk_gpio_disable_input(rst));
    BK_LOG_ON_ERR(bk_gpio_enable_output(rst));

    gpio_dev_unmap(clk);
    bk_gpio_set_capacity(clk, GPIO_DRIVER_CAPACITY_3);
    BK_LOG_ON_ERR(bk_gpio_disable_input(clk));
    BK_LOG_ON_ERR(bk_gpio_enable_output(clk));

    gpio_dev_unmap(csx);
    bk_gpio_set_capacity(csx, GPIO_DRIVER_CAPACITY_3);
    BK_LOG_ON_ERR(bk_gpio_disable_input(csx));
    BK_LOG_ON_ERR(bk_gpio_enable_output(csx));

    gpio_dev_unmap(sda);
    bk_gpio_set_capacity(sda, GPIO_DRIVER_CAPACITY_3);
    BK_LOG_ON_ERR(bk_gpio_disable_input(sda));
    BK_LOG_ON_ERR(bk_gpio_enable_output(sda));

    bk_gpio_set_output_high(clk);
    bk_gpio_set_output_high(csx);

    delay(200);
    LOGI("%s, complete\n", __func__);
}

static void rgb_bus_spi_send_data(uint8_t sda, uint8_t clk, uint8_t data)
{
    uint8_t n;

    //in while loop, to avoid disable IRQ too much time, release it if finish one byte.
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();

    for (n = 0; n < 8; n++)
    {
        if (data & 0x80)
        {
            bk_gpio_set_output_high(sda);
        }
        else
        {
            bk_gpio_set_output_low(sda);
        }

        delay(LCD_SPI_DELAY);
        data <<= 1;

        bk_gpio_set_output_low(clk);
        delay(LCD_SPI_DELAY);
        bk_gpio_set_output_high(clk);
        delay(LCD_SPI_DELAY);

    }

    GLOBAL_INT_RESTORE();
}

void rgb_bus_spi_write_cmd(uint8_t csx, uint8_t sda, uint8_t clk, uint8_t cmd)
{
    bk_gpio_set_output_low(csx);
    delay(LCD_SPI_DELAY);
    bk_gpio_set_output_low(sda);
    delay(LCD_SPI_DELAY);

    bk_gpio_set_output_low(clk);
    delay(LCD_SPI_DELAY);
    bk_gpio_set_output_high(clk);
    delay(LCD_SPI_DELAY);

    rgb_bus_spi_send_data(sda, clk, cmd);

    bk_gpio_set_output_high(csx);
    delay(LCD_SPI_DELAY);
}

void rgb_bus_spi_write_data(uint8_t csx, uint8_t sda, uint8_t clk, uint8_t data)
{
    bk_gpio_set_output_low(csx);
    delay(LCD_SPI_DELAY);
    bk_gpio_set_output_high(sda);
    delay(LCD_SPI_DELAY);

    bk_gpio_set_output_low(clk);
    delay(LCD_SPI_DELAY);
    bk_gpio_set_output_high(clk);
    delay(LCD_SPI_DELAY);

    rgb_bus_spi_send_data(sda, clk, data);

    bk_gpio_set_output_high(csx);
    delay(LCD_SPI_DELAY);
}

// Write 16-bit word command (high frequency format)
void rgb_bus_spi_write_hf_word_cmd(uint8_t csx, uint8_t sda, uint8_t clk, uint16_t cmd)
{
    bk_gpio_set_output_low(csx);
    delay(LCD_SPI_DELAY);

    rgb_bus_spi_send_data(sda, clk, 0x20);
    rgb_bus_spi_send_data(sda, clk, cmd >> 8);  // high 8bit
    rgb_bus_spi_send_data(sda, clk, 0x00);      // low 8bit
    rgb_bus_spi_send_data(sda, clk, cmd);       // cmd

    bk_gpio_set_output_high(csx);
    delay(LCD_SPI_DELAY);
}

// Write 16-bit word data (high frequency format)
void rgb_bus_spi_write_hf_word_data(uint8_t csx, uint8_t sda, uint8_t clk, uint16_t data)
{
    bk_gpio_set_output_low(csx);
    delay(LCD_SPI_DELAY);

    rgb_bus_spi_send_data(sda, clk, 0x40);
    rgb_bus_spi_send_data(sda, clk, data);

    bk_gpio_set_output_high(csx);
    delay(LCD_SPI_DELAY);
}

avdk_err_t bk_display_rgb_read(bk_display_bus_ctlr_t *controller, bk_display_bus_rw_type_t type, uint32_t cmd, void *param, size_t size)
{
    AVDK_RETURN_ON_FALSE(controller, BK_ERR_NULL_PARAM, TAG, "invalid bus controller handle");
    //rgb_bus_vn_ctlr_t *bus = __containerof(controller, rgb_bus_vn_ctlr_t, ops);


    return AVDK_ERR_OK;
}

avdk_err_t bk_display_rgb_write(bk_display_bus_ctlr_t *controller, bk_display_bus_rw_type_t type, uint32_t cmd, const void *param, size_t size)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    rgb_bus_vn_ctlr_t *bus = __containerof(controller, rgb_bus_vn_ctlr_t, ops);

    switch (type) {
        case BK_DISPLAY_BUS_RW_SPI_CMD:
            rgb_bus_spi_write_cmd(bus->config.csx_pin, bus->config.sda_pin, bus->config.clk_pin, cmd);
            break;
        case BK_DISPLAY_BUS_RW_SPI_DATA:
            rgb_bus_spi_write_data(bus->config.csx_pin, bus->config.sda_pin, bus->config.clk_pin, cmd);
            break;
        case BK_DISPLAY_BUS_RW_SPI_HF_CMD:
            rgb_bus_spi_write_hf_word_cmd(bus->config.csx_pin, bus->config.sda_pin, bus->config.clk_pin, cmd);
            break;
        case BK_DISPLAY_BUS_RW_SPI_HF_DATA:
            rgb_bus_spi_write_hf_word_data(bus->config.csx_pin, bus->config.sda_pin, bus->config.clk_pin, cmd);
            break;
        default:
            return AVDK_ERR_INVAL;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t bk_display_rgb_bus_enable(bk_display_bus_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    rgb_bus_vn_ctlr_t *bus = __containerof(controller, rgb_bus_vn_ctlr_t, ops);
    
    rgb_gpio_init();
    if(bus->config.clk_pin < 0 || bus->config.csx_pin < 0 || bus->config.sda_pin < 0)
    {
        LOGW("%s, invalid pin: reset: %d, clk: %d, csx: %d, sda: %d\n", __func__, bus->config.reset_pin, bus->config.clk_pin, bus->config.csx_pin, bus->config.sda_pin);
    }
    else
    {
        rgb_bus_spi_gpio_init(bus->config.sda_pin, bus->config.clk_pin, bus->config.csx_pin, bus->config.reset_pin);
    }
    return AVDK_ERR_OK;
}

static avdk_err_t bk_display_rgb_bus_disable(bk_display_bus_ctlr_t *controller)
{

    return AVDK_ERR_OK;
}

static avdk_err_t bk_display_rgb_bus_delete(bk_display_bus_ctlr_t *controller)
{

    return AVDK_ERR_OK;
}

avdk_err_t bk_display_rgb_bus_new(bk_display_bus_handle_t *handle, bk_display_rgb_bus_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    rgb_bus_vn_ctlr_t *bus = os_malloc(sizeof(rgb_bus_vn_ctlr_t));
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);

    os_memset(bus, 0, sizeof(rgb_bus_vn_ctlr_t));
    os_memcpy(&bus->config, config, sizeof(bk_display_rgb_bus_config_t));

    bus->ops.enable = bk_display_rgb_bus_enable;
    bus->ops.disable = bk_display_rgb_bus_disable;
    bus->ops.delete = bk_display_rgb_bus_delete;
    bus->ops.read = bk_display_rgb_read;
    bus->ops.write = bk_display_rgb_write;

    *handle = &(bus->ops);

    return AVDK_ERR_OK;
}