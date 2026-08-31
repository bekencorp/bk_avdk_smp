#include "VI5302_API.h"
#include "VI5302_User_Handle.h"
#include "VI5302_Algorithm.h"
#include "VI5302_System_Data.h"
#include <string.h>

#include <stdio.h>
#include "VI5302_Efuse.h"
VI5302_Params_T VI5302_Cali_Data;

// VI5302 GPIO中断信号
// 0-清零/没有中断信号
// 1-有中断信号
uint8_t VI5302_GPIO_Interrupt_status = 0;

uint8_t VI5302_Read_ChipVersion(uint8_t *Chip_Version)
{
    uint8_t chipid[3] = {0};
    uint8_t ret = 0;
    uint32_t Version = 0;

    ret = VI5302_IIC_Read_X_Bytes(REG_CHIPID_BASE, chipid, 3);
    Version = (chipid[1] << 16) + (chipid[0] << 8) + chipid[2];

    if (Version == 0x530200)
    {
        *Chip_Version = 0x02;
    }
    else
    {
        *Chip_Version = 0x00;
    }
    return ret;
}

uint8_t VI5302_Get_FW_Version(uint8_t *rVersion, uint8_t *len)
{
    uint8_t ret = 0;

    ret |= VI5302_IIC_Write_One_Byte(REG_USER_CMD, SUBCMD_READ_FW_VERSION);
    ret |= VI5302_IIC_Write_One_Byte(REG_CMD, CMD_USER_CFG);

    VI5302_Delay_Ms(5);

    ret |= VI5302_IIC_Read_One_Byte(REG_USER_DATA, len);
    ret |= VI5302_IIC_Read_X_Bytes(REG_USER_DATA + 1, rVersion, *len);

    return ret;
}

uint8_t VI5302_Set_Digital_Clock_Dutycycle(void)
{
    uint8_t ret = 0;

    ret |= VI5302_IIC_Write_One_Byte(REG_PW_CTRL, 0x0F);
    ret |= VI5302_IIC_Write_One_Byte(REG_PW_CTRL, 0x0E);
    VI5302_Delay_Ms(5);

    return ret;
}

uint8_t VI5302_Clear_Interrupt(void)
{
    uint8_t ret = 0;
    // 硬件中断
    VI5302_GPIO_Interrupt_status = 0;
    // 寄存器中断（软件中断）
    return VI5302_IIC_Read_One_Byte(REG_INT_STATUS, &ret);
}

uint8_t VI5302_Get_And_Clear_Interrupt(uint8_t *interrupt_status)
{
    uint8_t ret = 0, temp_status = 0;

    // 使用寄存器中断（软件中断）
    if (!VI5302_Cali_Data.VI5302_Interrupt_Mode_Status)
    {
        ret |= VI5302_IIC_Read_One_Byte(REG_INT_STATUS, &temp_status);
    }
    if (VI5302_GPIO_Interrupt_status || (temp_status & 0x01))
    {
        *interrupt_status = 0x01;
        VI5302_GPIO_Interrupt_status = 0;
    }
    else
    {
        *interrupt_status = 0x00;
    }
    return ret;
}

uint8_t VI5302_Start_Single_Ranging_Cmd(void)
{
    uint8_t ret = 0;

    ret |= VI5302_Clear_Interrupt();
    ret |= VI5302_IIC_Write_One_Byte(REG_CMD, CMD_SINGLE_RANGE);
    return ret;
}

uint8_t VI5302_Start_Continue_Ranging_Cmd(void)
{
    uint8_t ret = 0;

    ret |= VI5302_Clear_Interrupt();
    ret |= VI5302_IIC_Write_One_Byte(REG_CMD, CMD_CONTINOUS_RANGE);
    VI5302_Delay_Ms(5);
    return ret;
}

uint8_t VI5302_Stop_Continue_Ranging_Cmd(void)
{
    uint8_t ret = 0;
    ret |= VI5302_Clear_Interrupt();
    ret |= VI5302_IIC_Write_One_Byte(REG_CMD, CMD_STOP_RANGE);
    VI5302_Delay_Ms(5);
    return ret;
}

uint8_t VI5302_Chip_Register_Init()
{
    uint8_t ret = 0;

    // xshut复位
    VI5302_XSHUT_Enable(0);
    VI5302_Delay_Ms(10);
    VI5302_XSHUT_Enable(1);
    VI5302_Delay_Ms(10);

#if (VI5302_EXIT_75MHZ_MODE)
    VI5302_75MHz_Enable(1);                                      // 75M外部晶振
    ret |= VI5302_IIC_Write_One_Byte(REG_INTR_MASK, 0x01);       // 屏蔽内部中断
    ret |= VI5302_IIC_Write_One_Byte(REG_DTOP_5, 0x88 /*0x48*/); // 进入trim mode
#endif

    return ret;
}

uint8_t VI5302_Set_Keep_Alive(uint8_t value)
{
    uint8_t ret = 0;

    if (value)
        ret = VI5302_IIC_Write_One_Byte(REG_CMD, CMD_OPEN_KEEP_ALIVE);
    else
        ret = VI5302_IIC_Write_One_Byte(REG_CMD, CMD_CLOSE_KEEP_ALIVE);

    return ret;
}

uint8_t VI5302_Xtalk_Calibration(uint8_t *xtalk_buff)
{
    uint8_t status = 0;
    uint8_t ret = 0;
    uint16_t time_out_cnt = 1000;

    ret |= VI5302_Set_Sys_Temperature_Enable(0);
    ret |= VI5302_Clear_Interrupt();
    ret |= VI5302_IIC_Write_One_Byte(REG_CMD, CMD_XTALK);

    while (time_out_cnt--)
    {
        VI5302_Delay_Ms(10);
        ret |= VI5302_Get_And_Clear_Interrupt(&status);
        if (status)
        {
            VI5302_Delay_Ms(10);
            ret |= VI5302_IIC_Read_One_Byte(REG_SPECIAL_PURP1, &status);
            if (status == 0xAA)
            {
                ret |= VI5302_IIC_Read_X_Bytes(0x39, xtalk_buff, 5);
                break;
            }
            else
            {
                ret = Result_ERROR;
                break;
            }
        }
        if (time_out_cnt == 0)
        {
            ret = Result_ERROR;
            break;
        }
    }
    ret |= VI5302_Set_Sys_Temperature_Enable(1);
    return ret;
}

uint8_t VI5302_Offset_Calibration(uint16_t mili, float *ret_offset)
{
    uint8_t ret = 0;
    uint8_t get_data_total_times = 30; // 采集数据次数
    uint8_t start_get_data_times = 10; // 开始采集数据次数
    uint8_t get_data_cnt = 0;          // 采集数据计数
    uint8_t interrupt_status = 0;
    uint16_t abnormal_state_cnt = 0;
    uint8_t data_buff[64];
    float sum_tof = 0;

    uint32_t peak1 = 0, peak2 = 0, peak3 = 0, peak4 = 0;
    int16_t tof1 = 0, tof2 = 0, tof3 = 0, tof4 = 0;
    uint8_t tof1_bin = 0, tof2_bin = 0, tof3_bin = 0;
    uint32_t noise = 0;
    uint32_t multishot = 0;
    int16_t reftof = 0;
    uint8_t reftof_bin = 0;
    uint8_t misc_info = 0;
    uint8_t confidence1 = 0, confidence2 = 0, confidence3 = 0;
    uint32_t p[4];
    uint32_t r[3];
    uint8_t xtalk_bin = 0;
    uint32_t peak = 0;
    uint32_t noise_r = 0;
    int16_t tof = 0;
    int32_t bias = 0;
    uint8_t confidence = 0;
    uint32_t lower1 = 2000;
    uint32_t upper1 = 2300;
    uint32_t lower11 = 1210;
    uint32_t upper11 = 1300;
    uint32_t lower2 = 2800;
    uint32_t upper2 = 3000;
    uint32_t lower3 = 1900;
    uint32_t upper3 = 2100;

    uint32_t threshold = 18000;
    uint32_t c = 1500;

    ret = VI5302_Set_Sys_Temperature_Enable(0x00);
    ret |= VI5302_Start_Continue_Ranging_Cmd();

    do
    {
        ret |= VI5302_Get_And_Clear_Interrupt(&interrupt_status);

        if (!interrupt_status)
        {
            // 没有获取
            VI5302_Delay_Ms(1);
            abnormal_state_cnt++;
            if (abnormal_state_cnt > 1000)
            {
                // 异常
                ret = Result_ERROR;
                break;
            }
            continue;
        }
        abnormal_state_cnt = 0;

        if ((get_data_cnt >= start_get_data_times) && (get_data_cnt < get_data_total_times))
        {
            // 获取tof
            ret |= VI5302_IIC_Read_X_Bytes(0x0C, data_buff, 64);

            memcpy(&tof1, &data_buff[0], 2);
            memcpy(&peak1, &data_buff[2], 4);
            memcpy(&tof2, &data_buff[6], 2);
            memcpy(&peak2, &data_buff[8], 4);
            memcpy(&tof3, &data_buff[12], 2);
            memcpy(&peak3, &data_buff[14], 4);
            memcpy(&tof4, &data_buff[18], 2);
            memcpy(&peak4, &data_buff[20], 4);
            memcpy(&noise, &data_buff[24], 4);
            noise &= 0x00FFFFFF;
            memcpy(&multishot, &data_buff[27], 4);
            multishot &= 0x00FFFFFF;
            memcpy(&reftof, &data_buff[37], 2);
            reftof_bin = data_buff[39];
            tof1_bin = data_buff[40];
            tof2_bin = data_buff[41];
            tof3_bin = data_buff[42];
            misc_info = data_buff[44];

            peak = peak1;
            tof = tof1;

            bias = vi5302_calculate_pileup_bias(VI5302_Cali_Data.VI5302_MA_Sum, peak, noise, multishot);
            sum_tof += (tof + bias);
        }

        if (get_data_cnt < get_data_total_times)
        {
            get_data_cnt++;
        }
    } while (get_data_cnt < get_data_total_times);

    ret |= VI5302_Stop_Continue_Ranging_Cmd();
    ret |= VI5302_Set_Sys_Temperature_Enable(0x01);
    *ret_offset = (sum_tof) / ((get_data_total_times - start_get_data_times)) - mili;

    return ret;
}

uint8_t VI5302_Reftof_Calibration(uint16_t *ret_reftof)
{
    uint8_t ret = 0;
    uint8_t get_data_total_times = 30; // 采集数据次数
    uint8_t start_get_data_times = 10; // 开始采集数据次数
    uint8_t get_data_cnt = 0;          // 采集数据计数
    uint16_t abnormal_state_cnt = 0;
    uint8_t databuff[2];
    uint8_t interrupt_status = 0;
    uint16_t sum_reftof = 0;

    ret = VI5302_Set_Sys_Temperature_Enable(0x00);
    ret |= VI5302_Start_Continue_Ranging_Cmd();
    *ret_reftof = 0x00;
    do
    {
        ret |= VI5302_Get_And_Clear_Interrupt(&interrupt_status);
        if (!interrupt_status)
        {
            // 没有获取
            VI5302_Delay_Ms(1);
            abnormal_state_cnt++;
            if (abnormal_state_cnt > 2000)
            {
                // 异常
                return Result_ERROR;
            }
            continue;
        }
        abnormal_state_cnt = 0;

        if (get_data_cnt >= start_get_data_times && get_data_cnt < get_data_total_times)
        {
            // 获取tof
            ret |= VI5302_IIC_Read_X_Bytes(0x20, databuff, 2);
            sum_reftof += ((((int16_t)databuff[1]) << 8) | (((int16_t)databuff[0])));
        }

        if (get_data_cnt < get_data_total_times)
        {
            get_data_cnt++;
        }

    } while (get_data_cnt < get_data_total_times);
    ret |= VI5302_Stop_Continue_Ranging_Cmd();
    ret |= VI5302_Set_Sys_Temperature_Enable(0x01);

    *ret_reftof = (sum_reftof) / (get_data_total_times - start_get_data_times);
    return ret;
}

/**
 * @brief 获取直方图数据，共512字节，两个字节组成一个bin对应值
 * 
 * @param tdc 
 * @param histogram_buff 
 * @return uint8_t 
 */
uint8_t VI5302_Get_Histogram_Data(uint8_t tdc, uint8_t *histogram_buff)
{
    VI5302_Status ret = VI5302_OK;
    uint8_t i = 0;
    uint8_t reg_pw_ctrl = 0;
    uint8_t reg_sys_cfg = 0;
    uint16_t ram_addr_base = 0;
    uint8_t value = 0;
    VI5302_MEASURE_TypeDef result;

    ret |= VI5302_IIC_Read_One_Byte(REG_SYS_CFG, &value);
    ret |= VI5302_IIC_Write_One_Byte(REG_SYS_CFG, value & ~0x4);    // 关闭高级电源管理

    ret |= VI5302_Clear_Interrupt();
    ret |= VI5302_Start_Single_Ranging_Cmd();
    ret |= VI5302_Get_Measure_Data(&result, 1);

    if (tdc <= 9)
    {
        // 0-9直方图，9-ref直方图
        ram_addr_base = 0x0800 + 0x0200 * tdc;
    }
    else if (tdc >= 0xF1 && tdc <= 0xF4)
    {
        // CG(MA)直方图
        ram_addr_base = (tdc - 0xF1) * 0x0200;
    }
    ret |= VI5302_IIC_Read_One_Byte(REG_SYS_CFG, &reg_sys_cfg);
    ret |= VI5302_IIC_Write_One_Byte(REG_SYS_CFG, (reg_sys_cfg & (~((0x1 << 1)))) | (0x01 << 0));
    ret |= VI5302_IIC_Read_One_Byte(REG_PW_CTRL, &reg_pw_ctrl);
    ret |= VI5302_IIC_Write_One_Byte(REG_PW_CTRL, reg_pw_ctrl | (0x01 << 1));
    ret |= VI5302_IIC_Write_One_Byte(REG_MCU_CFG, 0x33);
    ret |= VI5302_IIC_Write_One_Byte(REG_CMD, CMD_SET_RAM_POINTER);
    ret |= VI5302_IIC_Write_One_Byte(REG_SIZE, 0x02);
    ret |= VI5302_IIC_Write_One_Byte(REG_SCRATCH_PAD_BASE + 0x00, *((uint8_t *)(&ram_addr_base)));
    ret |= VI5302_IIC_Write_One_Byte(REG_SCRATCH_PAD_BASE + 0x01, *((uint8_t *)(&ram_addr_base) + 1));

    VI5302_Delay_Ms(1);

    for (i = 0; i < 8; i++)
    {
        ret |= VI5302_IIC_Write_One_Byte(REG_CMD, CMD_READ_RAM);  // Issue RAM read command
        ret |= VI5302_IIC_Write_One_Byte(REG_SIZE, 0x40);
        VI5302_Delay_Ms(2);

        ret |= VI5302_IIC_Read_X_Bytes(REG_SCRATCH_PAD_BASE, &histogram_buff[64 * i], 64);
    }
    ret |= VI5302_IIC_Write_One_Byte(REG_MCU_CFG, 0x17);
    ret |= VI5302_IIC_Write_One_Byte(REG_SYS_CFG, (reg_sys_cfg & ~(0x01 << 0))); // clear sc_en
    ret |= VI5302_IIC_Write_One_Byte(REG_PW_CTRL, reg_pw_ctrl);

    ret |= VI5302_IIC_Write_One_Byte(REG_SYS_CFG, value);
    ret |= VI5302_IIC_Read_One_Byte(0x07, &value);
    ret |= VI5302_IIC_Write_One_Byte(0x07, value | 0x0F);

    return ret;
}

// wait_mode:1-在一定时间内等待中断信号，0-没有中断信号则直接退出
uint8_t VI5302_Internal_Get_Measure_Data(uint8_t *data_buff, VI5302_MEASURE_TypeDef *result, uint8_t wait_mode)
{
    uint8_t ma_sum = VI5302_Cali_Data.VI5302_MA_Sum;
    int16_t offset = (int16_t)VI5302_Cali_Data.VI5302_Cali_Offset;
    int8_t xtalk_pos = VI5302_Cali_Data.VI5302_Cali_CG_Pos;

    uint8_t ret = 0;
    uint8_t Interrupt_status = 0;
    uint16_t time_out_cnt = 2 * 1000;

	int16_t tof1 = 0, tof2 = 0, tof3 = 0, tof4 = 0;
    uint32_t peaks[4] = {0};
    uint32_t noise = 0;
    uint8_t tof_bins[4] = {0};
    uint32_t multishot = 0;
    uint16_t checksum0 = 0, checksum1 = 0;
    int16_t reftof = 0;
    uint8_t reftof_bin = 0;
    uint8_t misc_info = 0;
    uint8_t confidence_fw = 0;
    uint16_t ts = 0;
    uint8_t xtalk_bin = 0;
    int16_t tof = 0;
    int32_t bias = 0;
    uint8_t confidence = 0;
    uint8_t selected = 0;
    float correction_tof = 0;
    uint32_t peak = 0;
    uint32_t integral_times = 0;
    uint8_t flag = 0;
    uint32_t noise_r = 0;
	uint8_t confidences[3] = {0};
    uint32_t peakr = 0;


    while (time_out_cnt--)
    {
        VI5302_Delay_Ms(1);
        ret |= VI5302_Get_And_Clear_Interrupt(&Interrupt_status);
        if(ret)
        {
            return ret;
        }
        if (Interrupt_status)
        {
            ret |= VI5302_IIC_Read_X_Bytes(0x0C, data_buff, 64);
            if(ret)
            {
                return ret;
            }
			memcpy(&tof1, &data_buff[0], 2);//最高目标峰的距离
			memcpy(&peaks[0], &data_buff[2], 4);//最高目标峰的信号强度
			memcpy(&tof2, &data_buff[6], 2);//次高目标峰的距离
			memcpy(&peaks[1], &data_buff[8], 4);//次高目标峰的信号强度
			memcpy(&tof3, &data_buff[12], 2);
			memcpy(&peaks[2], &data_buff[14], 4);//第三目标峰信号强度
			memcpy(&tof4, &data_buff[18], 2);
			memcpy(&peaks[3], &data_buff[20], 4);//第四目标峰信号强度
			memcpy(&noise, &data_buff[24], 4);
			noise &= 0x00FFFFFF;
			memcpy(&integral_times, &data_buff[27], 4);//积分次数
			integral_times &= 0x00FFFFFF;
			reftof_bin = data_buff[39];
			tof_bins[0] = data_buff[40];//最高目标峰在直方图的横坐标位置
			tof_bins[1] = data_buff[41];//次高目标峰在直方图的横坐标位置
			tof_bins[2] = data_buff[42];//第三目标峰在直方图的横坐标位置
			flag = data_buff[44];

            noise_r = 100 * noise / 8;
			xtalk_bin = reftof_bin + xtalk_pos;
			if (tof_bins[1] <= (xtalk_bin + 2)) {
				peaks[1] = peaks[2];
				peaks[2] = peaks[3];
				tof2 = tof3;
				tof3 = tof4;
			}
            tof_bins[3] = xtalk_bin + 2;
			vi5302_calculate_confidence(peaks, tof_bins, noise_r, integral_times, ma_sum, flag, confidences);

			if (tof_bins[0] > (xtalk_bin + 2)) {
				tof = tof1;
				confidence = confidences[0];
				peak = peaks[0];
			} else if ((100 * peaks[0] - noise_r * ma_sum) / integral_times > 65) {
				tof = tof1;
				confidence = confidences[0];
				peak = peaks[0];
			} else {
				tof = tof2;
				confidence = confidences[1];
				peak = peaks[1];
			}

			bias = vi5302_calculate_pileup_bias(ma_sum, peak, noise, integral_times);
			bias += vi5302_calculate_noise_bias(noise, integral_times);
			correction_tof = tof + (int16_t)bias - offset;
			if (correction_tof > 6100)
				confidence = 0;
			if (noise > 25000 && (correction_tof > 1900 || correction_tof < -20))
				confidence = 0;
            peakr  = (peak - noise / 8 * ma_sum) * 16 / integral_times;
            if(noise_r > 18000 && peakr < 1)
                confidence = 0;

            if (result != NULL)
            {
                result->correction_tof = correction_tof;
                result->confidence = confidence;
                result->intecounts = integral_times;
            }

#ifdef INTERNAL_DEV
            memcpy(&data_buff[64], &correction_tof, 4);

            // confidence
            data_buff[68] = confidence;
            data_buff[70] = confidences[0];
            data_buff[71] = confidences[1];
            data_buff[72] = confidences[2];
            // flu_confidence
            // data_buff[30] = data4.confidence;
#endif
            break;
        }
        else
        {
            if (wait_mode == 0)
            {
                return Result_ERROR;
            }
        }

        if (time_out_cnt == 0)
        {
            return Result_ERROR;
        }
    }

    return ret;
}

uint8_t VI5302_Get_Measure_Data(VI5302_MEASURE_TypeDef *result, uint8_t wait_mode)
{
    uint8_t ret = 0;
    uint8_t data_buff[84] = {0};
    ret |= VI5302_Internal_Get_Measure_Data(data_buff, result, wait_mode);

    return ret;
}
