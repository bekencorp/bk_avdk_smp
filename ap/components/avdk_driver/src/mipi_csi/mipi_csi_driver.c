#include <os/os.h>
#include <os/mem.h>
#include <driver/int.h>
#include <driver/int_types.h>
#include <driver/mipi_csi.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include "sys_driver.h"

#define TAG "mipi_csi"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define BASEADDR_CSI            0x4c050000
#define BASEADDR_CSI_EXT        0x4c058000
#define GPIO_DEBUG_FUNC_VALUE   0x7F // 127
#define GPIO_DVP_FUNC_VALUE     0x81 // 129

static void bk_csi_isr(void)
{
    uint32_t state = REG_READ(BASEADDR_CSI + 0x0000000c);
    LOGI("%s, %d, state=%x\n", __func__, __LINE__, state);
}

bk_err_t bk_mipi_csi_controller_init(uint16_t width, uint16_t height, uint8_t data_type)
{
    uint8_t type = data_type; // default raw10

    // CSI PHY
    LOGI("%s %d %d format[0x%x] \r\n", __func__, width, height, data_type);

    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000040)) = 0xffffffff;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000044)) = 0xffffffff;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000008)) = 0xffffffff;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x000000c8)) = 0x0;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000300)) = 0x0;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000004)) = 0x01; // n_lanes = 2
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000010)) = 0x2c; // data ids
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000030)) = 0x0; // data id vc
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000018)) = 0x0; // ppi 8bit
    *((volatile unsigned int *)(BASEADDR_CSI + 0x000000ac)) = 0x0 << 24 | /*0x1f*/0x00 << 17 | 0x00<<16;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000080)) = 0x01<<24 | 0x01<<16 | 0x01<<8 | 0x00<<0;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000084)) = 0x0; // ipi vc = 0x00
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000088)) = type; // embedded data() | data type(0x2c)
    *((volatile unsigned int *)(BASEADDR_CSI + 0x0000008c)) = 0x01<<8; // autoAttr flush
#if 0 // test pattern enable
    uint16_t ipi_hsa = 0x08; // h sync pulse width(clock cycles of pixclk)
    uint16_t ipi_hbp = 0x0e; // h back porch width(clock cycles of pixclk)
    uint16_t ipi_hact = 640; // 640>>2 = 160;
    uint16_t ipi_hfp = 0x08;
    uint16_t ipi_hsd = 0x20; // h sync delay(clock cycles of pixclk)
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000090)) = ipi_hsa;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000094)) = ipi_hbp;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000098)) = ipi_hsd;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x0000009c)) = ipi_hsa + ipi_hbp + ipi_hact + ipi_hfp;
    uint16_t ipi_vsa = 0x08;
    uint16_t ipi_vbp = 0x02;
    uint16_t ipi_vact = 480; // 480>>2 = 120;
    uint16_t ipi_vfp = 0x02;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x000000b0)) = ipi_vsa; // vsa
    *((volatile unsigned int *)(BASEADDR_CSI + 0x000000b4)) = ipi_vbp; // vbp
    *((volatile unsigned int *)(BASEADDR_CSI + 0x000000bc)) = ipi_vact; // active line
    *((volatile unsigned int *)(BASEADDR_CSI + 0x000000b8)) = ipi_vfp; // vfp
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000800)) = 0x3;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000088)) = 0x24; // embedded data(disable) | data type(0x24)
    mipi_controller_tpg(ipi_vact, ipi_hact, 0x3FF);
#else
    uint16_t ipi_hsa = 0x20; // h sync pulse width(clock cycles of pixclk)
    uint16_t ipi_hbp = 0x10; // h back porch width(clock cycles of pixclk)
    uint16_t ipi_hact = width;// 1280;// 0x788; // 640>>2 = 160;
    uint16_t ipi_hfp = 0x10;
    uint16_t ipi_hsd = 0x50; // h sync delay(clock cycles of pixclk)
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000090)) = ipi_hsa;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000094)) = ipi_hbp;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000098)) = ipi_hsd;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x0000009c)) = ipi_hsa + ipi_hbp + ipi_hact + ipi_hfp;
    uint16_t ipi_vsa = 0x20;
    uint16_t ipi_vbp = 0x10;
    uint16_t ipi_vact = height; // 720;// 0x440; // 480>>2 = 120;
    uint16_t ipi_vfp = 0x10;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x000000b0)) = ipi_vsa; // vsa
    *((volatile unsigned int *)(BASEADDR_CSI + 0x000000b4)) = ipi_vbp; // vbp
    *((volatile unsigned int *)(BASEADDR_CSI + 0x000000bc)) = ipi_vact; // active line
    *((volatile unsigned int *)(BASEADDR_CSI + 0x000000b8)) = ipi_vfp; // vfp
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000800)) = 0x3;
#endif

    return BK_OK;
}

void bk_mipi_csi_controller_deinit(void)
{
    LOGI("mipi controller deinit\r\n");
    // *((volatile unsigned int *) (BASEADDR_CSI + 0x00004000)) = 0x3;  //config naneng dphy
    // *((volatile unsigned int *)(BASEADDR_CSI + 0x00000040)) = 0x0;
    // *((volatile unsigned int *)(BASEADDR_CSI + 0x00000044)) = 0x0;
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000008)) = 0x0;
}

extern void bk_delay_us(UINT32 us);
void bk_mipi_csi_controller_reset(void)
{
    // reset phy first
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000044)) = 0x0;
    //bk_delay_us(5);
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000044)) = 0x1;
    // reset csi host
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000008)) = 0x0;
    //bk_delay_us(5);
    *((volatile unsigned int *)(BASEADDR_CSI + 0x00000008)) = 0x1;
}

void bk_mipi_csi_phy_term_set(uint32_t v1, uint32_t v2)
{
    bk_mipi_csi_controller_reset();
}

void bk_mipi_csi_ext_set_enable(uint8_t mode)
{
    if (mode == 0)
    {
        // csi mode
        REG_WRITE(BASEADDR_CSI_EXT + 0x00000000, 0x3);
    }
    else
    {
        // dvp mode
        REG_WRITE(BASEADDR_CSI_EXT + 0x00000000, 0x1);
    }
}

void bk_dvp_io_config(void)
{
    /* dvp pins function config */
    for (int i = GPIO_29; i <= GPIO_39; i++) {
        gpio_dev_unmap(i);
        //gpio_dev_map(i, GPIO_DVP_FUNC_VALUE);
        *(volatile uint32_t*)(SOC_AON_GPIO_REG_BASE + i * 4) |= (GPIO_DVP_FUNC_VALUE << 24); // gpio dvp function map
    }
}

void bk_mipi_csi_enable_debug_pin(void)
{
    LOGI("%s\r\n", __func__);
    for (int i = GPIO_2; i <= GPIO_4; i++) {
        gpio_dev_unmap(i);
        //gpio_dev_map(i, GPIO_DEBUG_FUNC_VALUE);
        *(volatile uint32_t*)(SOC_AON_GPIO_REG_BASE + i * 4) |= (GPIO_DEBUG_FUNC_VALUE << 24); // gpio debug function map
    }

    uint32_t reg = REG_READ(BASEADDR_CSI_EXT + 0x00000000);
    reg |= 0x40;// debug for 0x40
    REG_WRITE(BASEADDR_CSI_EXT + 0x00000000, reg);

    REG_WRITE(SOC_SYSTEM_REG_BASE + 0x39 * 4, 6); //mipi debug en

    reg = REG_READ(SOC_SYS_AHBP_REG_BASE + 0x23 * 4);
    reg |= 0x1;
    REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x23 * 4, reg); //mipi debug mode

    bk_int_isr_register(INT_SRC_CSI, bk_csi_isr, NULL);
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_CSI, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_CSI, 1);
#endif
}

void bk_mipi_csi_disable_debug_pin(void)
{
    LOGI("%s\r\n", __func__);
    REG_WRITE(SOC_SYSTEM_REG_BASE + 0x39 * 4, 0); //mipi debug disable
}