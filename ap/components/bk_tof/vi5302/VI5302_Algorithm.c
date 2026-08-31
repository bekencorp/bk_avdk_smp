#include "VI5302_Algorithm.h"
#include "VI5302_API.h"
#include "stdlib.h"

/**
 * @brief 计算校验和
 * @param data_buff 数据缓冲区
 * @param len 数据长度
 * @return 校验和
 */
uint16_t VI5302_Calculate_Checksum(uint8_t *data_buff, int len)
{
    uint8_t i;
    uint16_t checksum = 0;

    for (i = 0; i < len; ++i)
        checksum += data_buff[i];

    return checksum;
}

/**
 * @brief Pile-up 补偿，正常测距、Offset 标定，修正 TOF 偏短
 * @brief 算法： 去噪算有效 peak → 查表 xth[]/pth[] → 分段线性插值 得 bias（mm 级整数）
 * @brief 计算堆叠偏差
 * @param ma_sum 移动平均和
 * @param peak 峰值
 * @param noise 噪声
 * @param integral_times 积分次数
 * @return 堆叠偏差
 */
int32_t vi5302_calculate_pileup_bias(uint8_t ma_sum, uint32_t peak, uint32_t noise, uint32_t integral_times)
{
	int32_t xth[] = {0,363,500,739,1000,1240,1608,1755,1924,2090,2400};
	int32_t pth[] = {0,5,10,15,23,28,44,47,56,66,75};
	int i = 0;
	int32_t bias = 0;
	int32_t peak_tmp = 0;

	if (integral_times == 0)
		return bias;

	noise /= 8;
	if (peak > noise * ma_sum)
		peak_tmp = (peak - noise * ma_sum) * 16 / integral_times;

	for (i = 0;i < sizeof(xth) / sizeof(xth[0]) - 1; i++) {
		if (peak_tmp < xth[i+1]) {
			bias = (pth[i + 1] - pth[i]) * (peak_tmp - xth[i]) / (xth[i + 1] - xth[i]) + pth[i];
			return bias;
		}
	}
	bias = (pth[i] - pth[i - 1]) * (peak_tmp - xth[i - 1]) / (xth[i] - xth[i - 1]) + pth[i - 1];
	return bias;
}


/**
 * @brief 噪声补偿，与 pile-up bias 相加 后用于最终距离校正，修正噪声引起误差
 * @brief 算法：oise_cal = 1000 * noise / 8 / integral_times，再查 {0,10,25,45} 阈值表插值
 * @param noise 噪声
 * @param integral_times 积分次数
 * @return 噪声偏差
 */
int32_t vi5302_calculate_noise_bias(uint32_t noise, uint32_t integral_times)
{
    int32_t xth[] = {0,10,25,45};
    int32_t pth[] = {0,5,6,6};

    uint8_t len = 0;
    uint8_t i = 0;
    int16_t bias = 0;
    int32_t noise_cal = 0;

    len = sizeof(xth) / sizeof(xth[0]);
    if (integral_times == 0)
        return bias;

    noise_cal = 1000 * noise / 8 / integral_times;
    for (i = 0; i < len - 1; i++) {
        if (noise_cal < xth[i + 1]) {
            bias = (pth[i + 1] - pth[i]) * (noise_cal - xth[i]) / (xth[i + 1] - xth[i]) + pth[i];
            return bias;
        }
    }
    bias = (pth[i] - pth[i - 1]) * (noise_cal - xth[i - 1]) / (xth[i] - xth[i - 1]) + pth[i - 1];
    return bias;
}

#define LOWER1		2000
#define UPPER1		2300
#define LOWER11		1210
#define UPPER11		1300
#define LOWER2		2800
#define UPPER2		3000
#define LOWER3		1900
#define UPPER3		2100

/*
 * @brief 计算置信度，单峰置信度（内部）
 * @param tof_bin 光子计数
 * @param flag 标志位
 * @param r 相邻峰强度比
 * @param upper 上限
 * @param lower 下限
 * @param c 系数
 * @param threshold 阈值
 */
uint8_t vi5302_calculate_confidence_internal(uint8_t tof_bin, uint8_t flag, uint32_t r, uint32_t upper, uint32_t lower, int c, uint8_t threshold)
{
	uint32_t level = 0;

	if (flag == 0)
		return 0;

	if (c > 0 && tof_bin < threshold)
		level = 0;
	else if (c < 0 && tof_bin > threshold)
		level = 0;
	else
		level = c * (tof_bin - threshold) / 10;

	lower += level;
	upper += level;
	// if (dev->enable_debug)
	// 	vi5302_infomsg("upper:%u, lower:%u, level:%u, r:%u\n", upper, lower, level, r);

	if (r > upper)
		return 100;
	else if (r < lower)
		return 0;
	else
		return 100 * (r - lower) / (upper - lower);
}

/**
 * @brief 计算置信度，多峰置信度，判断测距是否可信
 * @brief 对最多 4 个目标峰算 3 路置信度 confidences[0..2]
 * @param peaks 峰值
 * @param tof_bins 光子计数
 * @param noise_r 噪声
 * @param integral_times 积分次数
 * @param ma_sum 移动平均和
 * @param flag 标志位
 * @param confidences 置信度
 */
void vi5302_calculate_confidence(uint32_t *peaks, uint8_t * tof_bins, uint32_t noise_r, uint32_t integral_times, uint32_t ma_sum, uint8_t flag, uint8_t *confidences)
{
    uint64_t p[4] = {0};
    uint32_t r[3] = {0};
    uint32_t lower1= LOWER1;
    uint32_t upper1 = UPPER1;

	p[0] = 100 * (uint64_t)peaks[0] / ma_sum - noise_r;
	p[1] = 100 * (uint64_t)peaks[1] / ma_sum - noise_r;
	p[2] = 100 * (uint64_t)peaks[2] / ma_sum - noise_r;
	p[3] = 100 * (uint64_t)peaks[3] / ma_sum - noise_r;
	r[0] = 1000 * p[0] / p[1];
	r[1] = 1000 * p[1] / p[2];
	r[2] = 1000 * p[2] / p[3];

    if (noise_r > 18000) {
		upper1 = UPPER11;
		lower1 = LOWER11;
	}
	confidences[0] = vi5302_calculate_confidence_internal(tof_bins[0], flag & 0x1, r[0], upper1, lower1, 35, 95);
	confidences[1] = vi5302_calculate_confidence_internal(tof_bins[1], flag & 0x2, r[1], UPPER2, LOWER2, 50, 95);
	confidences[2] = vi5302_calculate_confidence_internal(tof_bins[2], flag & 0x4, r[2], UPPER3, LOWER3, 30, 90);

	if (peaks[0] < 500)
		confidences[0] = 0;
	if (peaks[1] < 500)
		confidences[1] = 0;
	if (peaks[2] < 500)
		confidences[2] = 0;

	if ((flag & 0x2) && (flag & 0x4) && (confidences[2] >= 80))
		confidences[1] = 100;
	if (confidences[1] == 100 && confidences[2] == 100)
		confidences[0] = 100;
	if (r[1] > 14000)
		confidences[0] = 100;
	if ((tof_bins[1] > tof_bins[3]) && (confidences[1] == 100))
		confidences[0] = 100;
}
