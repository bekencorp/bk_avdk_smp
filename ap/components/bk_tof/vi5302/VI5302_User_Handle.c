/*
 * BK7259 port of Visionics VI5302_User_Handle.c
 * Only this file is platform-specific; API/Firmware/Algorithm stay vendor code.
 */
#include <os/os.h>
#include <driver/gpio.h>
#include <gpio_driver.h>
#include <driver/i2c.h>
#include <components/log.h>

#include <string.h>
#include "VI5302_User_Handle.h"
#include "VI5302_Firmware.h"
#include "VI5302_System_Data.h"
#include "VI5302_API.h"


/***********************************************************
 * 此文档需要客户移植完成
 * 1、IIC读写函数
 * 2、实现延时函数：void VI5302_Delay_Ms(uint16_t nMs)
 * 3、如果使用硬件中断,则在触发下降沿中断中调用：void VI5302_GPIO_Interrupt_Handle(void)
 * 4、XSHUT高低电位的控制：void VI5302_XSHUT_Enable(uint8_t state)
 * ********************************************************/
#include <bk_tof_vi5302.h>

/* Beken I2C FIFO ISR is more reliable with smaller memory transactions.
 * Firmware download and measure dump use 64B; split to avoid mid-xfer hangs. */
#define VI5302_I2C_CHUNK_MAX    16
#define VI5302_I2C_TIMEOUT_MS   200

static uint8_t vi5302_dev_addr_7bit(uint8_t addr_8bit)
{
	return (uint8_t)(addr_8bit >> 1);
}

static uint8_t i2c_mem_xfer(uint8_t is_write, uint8_t dev_addr, uint16_t addr,
			    uint8_t *buf, uint16_t tlen)
{
	i2c_mem_param_t mem = {0};

	mem.dev_addr = vi5302_dev_addr_7bit(dev_addr);
	mem.mem_addr = addr;
	mem.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
	mem.data = buf;
	mem.data_size = tlen;
	mem.timeout_ms = VI5302_I2C_TIMEOUT_MS;

	if (is_write)
		return (bk_i2c_memory_write(TOF_I2C_ID, &mem) == BK_OK) ? 0 : 1;
	return (bk_i2c_memory_read(TOF_I2C_ID, &mem) == BK_OK) ? 0 : 1;
}

uint8_t IIC_Write_X_Bytes(uint8_t dev_addr, uint16_t addr, uint8_t *pValue, uint16_t tlen)
{
	if (pValue == NULL || tlen == 0)
		return 1;

	while (tlen) {
		uint16_t n = (tlen > VI5302_I2C_CHUNK_MAX) ? VI5302_I2C_CHUNK_MAX : tlen;

		if (i2c_mem_xfer(1, dev_addr, addr, pValue, n))
			return 1;
		addr += n;
		pValue += n;
		tlen -= n;
	}
	return 0;
}

uint8_t IIC_Read_X_Bytes(uint8_t dev_addr, uint16_t addr, uint8_t *value, uint16_t tlen)
{
	if (value == NULL || tlen == 0)
		return 1;

	while (tlen) {
		uint16_t n = (tlen > VI5302_I2C_CHUNK_MAX) ? VI5302_I2C_CHUNK_MAX : tlen;

		if (i2c_mem_xfer(0, dev_addr, addr, value, n))
			return 1;
		addr += n;
		value += n;
		tlen -= n;
	}
	return 0;
}

uint8_t VI5302_IIC_Read_One_Byte(uint16_t addr, uint8_t *value)
{
	return IIC_Read_X_Bytes(VI5302_IIC_DEV_ADDR, addr, value, 1);
}

uint8_t VI5302_IIC_Read_X_Bytes(uint16_t addr, uint8_t *value, uint16_t tlen)
{
	return IIC_Read_X_Bytes(VI5302_IIC_DEV_ADDR, addr, value, tlen);
}

uint8_t VI5302_IIC_Write_One_Byte(uint16_t addr, uint8_t value)
{
	return IIC_Write_X_Bytes(VI5302_IIC_DEV_ADDR, addr, &value, 1);
}

uint8_t VI5302_IIC_Write_X_Bytes(uint16_t addr, uint8_t *pValue, uint16_t tlen)
{
	return IIC_Write_X_Bytes(VI5302_IIC_DEV_ADDR, addr, pValue, tlen);
}

/*****************************
***如用写OTP功能,则以下需要用户实现
***如不用写OTP功能,则以下函数空实现
*****************************/
// 7.5v接入芯片-OTP写入数据时需要使用
void VI5302_7V5_Enable(uint8_t state)
{
	(void)state;
	/* OTP programming only; not used on robot board */
}

void VI5302_Delay_Ms(uint16_t nMs)
{
	rtos_delay_milliseconds(nMs);
}

void VI5302_GPIO_Interrupt_Handle(void)
{
	if (VI5302_Cali_Data.VI5302_Interrupt_Mode_Status)
	{
		VI5302_GPIO_Interrupt_status = 1;
	}
}

void VI5302_XSHUT_Enable(uint8_t state)
{
	if (state)
		bk_gpio_set_output_high(TOF_XSHUT_PIN);
	else
		bk_gpio_set_output_low(TOF_XSHUT_PIN);
}

/*@brief demo示例，客户需要根据实际硬件平台进行适配*/
#if 0
void VI5302_main(void)
{
    uint8_t ret = 0;
    uint8_t data_buff[10];
    VI5302_MEASURE_TypeDef result = {0};

    // 1、IIC 初始化
    // 支持1M

    // 2、GPIO 初始化
    // （a）驱动配置xshut管脚----上拉输出
    // （b）如果使用硬件GPIO中断，则驱动配置GPIO管脚----下降沿输入中断

    // 3、选择中断方式：0----寄存器中断，其他值----硬件中断
    VI5302_Cali_Data.VI5302_Interrupt_Mode_Status = 0x00; // 寄存器中断
//		VI5302_Cali_Data.VI5302_Interrupt_Mode_Status = 0x88; // 硬件中断

    // 5、VI5302寄存器初始化
    ret |= VI5302_Chip_Register_Init();

    // 6、VI5302固件写入
    ret |= VI5302_Download_Firmware((uint8_t *)VI5302_firmware_buff, VI5302_FirmwareSize());
		
		ret |= VI5302_Set_Integralcounts_Frame(30, 131072);
    /******************************** 标定 S ********************************/
    // xtalk标定
    ret |= VI5302_Xtalk_Calibration(data_buff);
    VI5302_Cali_Data.VI5302_Cali_CG_Pos = data_buff[0];
    // 将xtalk_pos参数写到固件
    ret |= VI5302_Set_Sys_Xtalk_Position(VI5302_Cali_Data.VI5302_Cali_CG_Pos);

    // offset标定
    ret |= VI5302_Offset_Calibration(OFFSET_CALI_DISTANCE, &VI5302_Cali_Data.VI5302_Cali_Offset);
    if (ret == 0)
    {
        printf("VI5302_Cali_Offset = %f\r\n", VI5302_Cali_Data.VI5302_Cali_Offset);
    }

    // 模组只需做一次标定即可，将标定结果保存到flash中，之后读取赋值
    // 需要保存的数据
    // VI5302_Cali_Data.VI5302_Cali_CG_Pos
    // VI5302_Cali_Data.VI5302_Cali_Offset

    /******************************** 标定 E ********************************/

    /************************* 标定后正常测距流程 *****************************/

    // 从flash中读取标定结果示例，平台相关，需客户实现
    // VI5302_Cali_Data.VI5302_Cali_CG_Pos = flash_data.VI5302_Cali_CG_Pos;
    // VI5302_Cali_Data.VI5302_Cali_Offset = flash_data.VI5302_Cali_Offset;

    // 将xtalk_pos参数写到固件，offset值驱动使用，无需再写到固件
    ret |= VI5302_Set_Sys_Xtalk_Position(VI5302_Cali_Data.VI5302_Cali_CG_Pos);

    // 开温补，固件默认开
    // ret |= VI5302_Set_Sys_Temperature_Enable(0x01);

#if 0
        // 获取直方图数据示例
        uint8_t histogram_buff[512] = {0};
        while (1)
        {
            ret = VI5302_Get_Histogram_Data(0xF2, histogram_buff);
            if (!ret)
            {
                for (int bin = 0; bin < 256; bin++)
                {
                    uint16_t value =
                        ((uint16_t)histogram_buff[bin * 2 + 1]) |
                        ((uint16_t)histogram_buff[bin * 2] << 8);

                    printf("%u ", value);
                }
                printf("\n");
            }
            VI5302_Delay_Ms(1);
        }
#endif

    if (ret)
    {
        printf("VI5302 Configer Error\r\n");
    }
    else
    {
        printf("VI5302 Configer Ok\r\n");
    }

    // ret |= VI5302_Start_Single_Ranging_Cmd();
    ret |= VI5302_Start_Continue_Ranging_Cmd();
    while (1)
    {
        ret |= VI5302_Get_Measure_Data(&result, 1);
        if (!ret)
        {
            printf("tof = %4d, confidece = %2d, intecounts = %d\r\n", result.correction_tof, result.confidence, result.intecounts);
        }

        VI5302_Delay_Ms(1);
    }
}
#endif
