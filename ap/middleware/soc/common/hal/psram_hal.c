//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <common/bk_include.h>
#include <driver/psram_types.h>
#include "psram_hal.h"
#include "psram_ll_macro_def.h"
#include "hal_port.h"
#include "bk_pm_internal_api.h"
#include "sys_driver.h"
#include <modules/pm.h>
#include "aon_pmu_ll.h"
#include <driver/psram.h>
#include <modules/chip_support.h>
#if (CONFIG_PSRAM_INTERLEAVE)
#include "sys_hal.h"
#endif

extern void bk_delay_us(uint32_t us);

static void psram_delay(volatile uint32_t times)
{
	while(times--);
}

void psram_hal_set_sf_reset(uint32_t value)
{
	psram_ll_set_sf_reset_value(value);
}

void psram_hal_set_cmd_reset(void)
{
	psram_ll_set_reg8_value(0x4);
}

void psram_hal_cmd_write(uint32_t addr, uint32_t value)
{
	psram_ll_set_write_address(addr);
	psram_ll_set_write_data(value);
	psram_ll_set_reg8_value(0x1);
	while(psram_ll_get_reg8_value() & 0x1);
}

uint32_t psram_hal_cmd_read(uint32_t addr)
{
	uint8_t m = 10, i = 5;

	for (i = 5; i > 0; i--)
	{
		psram_ll_set_write_address(addr);
		psram_ll_set_reg8_value(0x2);

		m = 10;

		while(psram_ll_get_reg8_value() & 0x2)
		{
			for (int j = 0; j < 5000; j++) {}

			m--;

			if (m == 0)
				break;
		};

		if (m != 0)
		{
			return psram_ll_get_regb_value();
		}

		psram_hal_set_sf_reset(0);
		psram_hal_set_sf_reset(1);
		psram_hal_set_cmd_reset();
	}

	return 0;
}

void psram_hal_set_clk(psram_clk_t clk)
{
	switch (clk)
	{
		case PSRAM_240M:
			sys_drv_psram_clk_sel(1);	 // clk sel: 0-320 1-480
			sys_drv_psram_set_clkdiv(0); //frq:  F/(2 + (1+div))
			break;
		case PSRAM_160M:
			sys_drv_psram_clk_sel(0);	 // clk sel: 0-320 1-480
			sys_drv_psram_set_clkdiv(0); //frq:  F/(2 + (1+div))
			break;
		case PSRAM_120M:
			sys_drv_psram_clk_sel(1);	 // clk sel: 0-320 1-480
			sys_drv_psram_set_clkdiv(1); //frq:  F/(2 + (1+div))
			break;
		case PSRAM_80M:
			sys_drv_psram_clk_sel(0);	 // clk sel: 0-320 1-480
			sys_drv_psram_set_clkdiv(1); //frq:  F/(2 + (1+div))
			break;
		default:
			break;
	}
}

void psram_hal_set_voltage(psram_voltage_t voltage)
{
#if (COINFG_SOC_BK7256XX)
	switch (voltage)
	{
		case PSRAM_OUT_3_20V:
			sys_drv_psram_psldo_vset(0, 1);
			break;
		case PSRAM_OUT_3_0V:
			sys_drv_psram_psldo_vset(0, 0);
			break;
		case PSRAM_OUT_2_0V:
			sys_drv_psram_psldo_vset(1, 1);
			break;
		case PSRAM_OUT_1_80V:
			sys_drv_psram_psldo_vset(1, 0);
			break;
		default:
			break;
	}
#else
	switch (voltage)
	{
		case PSRAM_OUT_3_0V:
			sys_drv_psram_psldo_vset(0, 0);
			break;
		case PSRAM_OUT_1_95V:
			sys_drv_psram_psldo_vset(1, 0);
			break;
		case PSRAM_OUT_1_90V:
			sys_drv_psram_psldo_vset(1, 1);
			break;
		case PSRAM_OUT_1_85V:
			sys_drv_psram_psldo_vset(1, 2);
			break;
		case PSRAM_OUT_1_80V:
			sys_drv_psram_psldo_vset(1, 3);
			break;
		default:
			break;
	}
#endif
}

/* Generic check: true if addr is not within any valid PSRAM range (non-interleave or interleave). */
static inline bool is_psram_addr_out_of_range(uint32_t addr)
{
#if CONFIG_PSRAM_INTERLEAVE
	/* Interleave mode: single virtual space [SOC_PSRAM0_DATA_BASE, + 2*SIZE) */
	uint32_t virt_base = (uint32_t)SOC_PSRAM0_DATA_BASE;
	if (addr >= virt_base && addr < virt_base + 2 * SOC_PSRAM_DATA_SIZE)
		return false;
#endif
	bool in_psram0 = (addr >= SOC_PSRAM_DATA_BASE) && (addr < (SOC_PSRAM_DATA_BASE + SOC_PSRAM_DATA_SIZE));
	bool in_psram1 = (addr >= SOC_PSRAM1_DATA_BASE) && (addr < (SOC_PSRAM1_DATA_BASE + SOC_PSRAM_DATA_SIZE));
	return !(in_psram0 || in_psram1);
}

static inline bool is_32bytes_aligned(uint32_t addr)
{
	return ((addr & 0x1f) == 0);
}

static inline bool is_64bytes_aligned(uint32_t addr)
{
	return ((addr & 0x3f) == 0);
}

#if (CONFIG_PSRAM_WRITE_THROUGH)

#if CONFIG_PSRAM_INTERLEAVE
/** Configure write-through for a range in the interleave virtual address space (0x8...). */
static int psram_hal_set_write_through_interleave(psram_write_through_area_t area, uint32_t start, uint32_t end)
{
	uint32_t virt_base = (uint32_t)SOC_PSRAM0_DATA_BASE;
	uint32_t step_reg = sys_hal_get_psram_interleave_config();
	uint32_t step_bytes = 256 >> step_reg; /* 0:256B, 1:128B, 2:64B, 3:32B */
	uint32_t block_start = (start - virt_base) / step_bytes;
	uint32_t block_end = (end - virt_base - 1) / step_bytes;
	uint32_t hal_id = (uint32_t)area;

	/* PSRAM0: even block indices; PSRAM1: odd block indices */
	uint32_t first_even = block_start + (block_start & 1);
	uint32_t last_even = block_end - (block_end & 1);
	if (first_even <= last_even) {
		uint32_t off_start = (first_even / 2) * step_bytes;
		uint32_t off_end = (last_even / 2 + 1) * step_bytes;
		uint32_t cfg_start = off_start >> 5;
		uint32_t cfg_end = off_end >> 5;
		psram_ll_set_cover_start_with_base((uint32_t)SOC_PSRAM0_REG_BASE, hal_id, cfg_start);
		psram_ll_set_cover_stop_enable_with_base((uint32_t)SOC_PSRAM0_REG_BASE, hal_id, BIT(31) | cfg_end);
		HAL_LOGV("wt interleave PSRAM0: area=%u hal_id=%u cfg_start=0x%x cfg_end=0x%x\r\n",
			(unsigned)area, (unsigned)hal_id, (unsigned)cfg_start, (unsigned)cfg_end);
	}
	uint32_t first_odd = (block_start & 1) ? block_start : (block_start + 1);
	uint32_t last_odd = (block_end & 1) ? block_end : (block_end - 1);
	if (first_odd <= last_odd) {
		uint32_t off_start = (first_odd / 2) * step_bytes;
		uint32_t off_end = (last_odd / 2 + 1) * step_bytes;
		uint32_t cfg_start = off_start >> 5;
		uint32_t cfg_end = off_end >> 5;
		psram_ll_set_cover_start_with_base((uint32_t)SOC_PSRAM1_REG_BASE, hal_id, cfg_start);
		psram_ll_set_cover_stop_enable_with_base((uint32_t)SOC_PSRAM1_REG_BASE, hal_id, BIT(31) | cfg_end);
		HAL_LOGV("wt interleave PSRAM1: area=%u hal_id=%u cfg_start=0x%x cfg_end=0x%x\r\n",
			(unsigned)area, (unsigned)hal_id, (unsigned)cfg_start, (unsigned)cfg_end);
	}
	return BK_OK;
}
#endif

/** Configure write-through for a range in non-interleaved PSRAM (0x60.../0x64...). Controller chosen by start address. */
static int psram_hal_set_write_through_non_interleave(psram_write_through_area_t area, uint32_t start, uint32_t end)
{
	uint32_t reg_base, data_base, cfg_start, cfg_end;
	uint32_t hal_id = area % PSRAM_WRITE_THROUGH_AREA_PER_PSRAM;

	if (start >= (uint32_t)SOC_PSRAM0_DATA_BASE &&
	    start < (uint32_t)SOC_PSRAM0_DATA_BASE + SOC_PSRAM_DATA_SIZE) {
		reg_base = (uint32_t)SOC_PSRAM0_REG_BASE;
		data_base = (uint32_t)SOC_PSRAM0_DATA_BASE;
	} else {
		reg_base = (uint32_t)SOC_PSRAM1_REG_BASE;
		data_base = (uint32_t)SOC_PSRAM1_DATA_BASE;
	}
	cfg_start = start >> 5;
	cfg_end   = end >> 5;
	psram_ll_set_cover_start_with_base(reg_base, hal_id, cfg_start);
	psram_ll_set_cover_stop_enable_with_base(reg_base, hal_id, BIT(31) | cfg_end);
	{
		uint32_t rb_start = psram_ll_get_cover_start_with_base(reg_base, hal_id);
		uint32_t rb_stop  = psram_ll_get_cover_stop_enable_with_base(reg_base, hal_id);
		HAL_LOGV("wt cover: area=%u hal_id=%u reg_base=0x%x data_base=0x%x\r\n",
			(unsigned)area, (unsigned)hal_id, (unsigned)reg_base, (unsigned)data_base);
		HAL_LOGV("  set start=0x%x end=0x%x (32B unit) | readback start=0x%x stop_ena=0x%x\r\n",
			(unsigned)cfg_start, (unsigned)cfg_end, (unsigned)rb_start, (unsigned)rb_stop);
	}
	return BK_OK;
}

/** Clear write-through for an area on both PSRAM0 and PSRAM1. */
static void psram_hal_disable_write_through_area(uint32_t hal_id)
{
	psram_ll_set_cover_stop_enable_with_base((uint32_t)SOC_PSRAM0_REG_BASE, hal_id, 0);
	psram_ll_set_cover_stop_enable_with_base((uint32_t)SOC_PSRAM1_REG_BASE, hal_id, 0);
}

#endif /* CONFIG_PSRAM_WRITE_THROUGH */

int psram_hal_set_write_through(psram_write_through_area_t area, uint32_t enable, uint32_t start, uint32_t end)
{
#if (CONFIG_PSRAM_WRITE_THROUGH)

	if (area > PSRAM_WRITE_THROUGH_AREA_COUNT) {
		return BK_ERR_PSRAM_AREA;
	}

	if (enable) {
		if (start >= end) {
			return BK_ERR_PSRAM_ADDR_RELATION;
		}
		if (is_psram_addr_out_of_range(start) || is_psram_addr_out_of_range(end)) {
			return BK_ERR_PSRAM_ADDR_OUT_OF_RANGE;
		}
		if ((!is_64bytes_aligned(start)) || (!is_64bytes_aligned(end))) {
			return BK_ERR_PSRAM_ADDR_ALIGN;
		}

#if CONFIG_PSRAM_INTERLEAVE
		{
			uint32_t virt_base = (uint32_t)SOC_PSRAM0_DATA_BASE;
			uint32_t virt_size = 2 * SOC_PSRAM_DATA_SIZE;
			if (start >= virt_base && start < virt_base + virt_size) {
				if (area >= PSRAM_WRITE_THROUGH_AREA_PER_PSRAM) {
					return BK_ERR_PSRAM_AREA;
				}
				return psram_hal_set_write_through_interleave(area, start, end);
			}
		}
#endif
		return psram_hal_set_write_through_non_interleave(area, start, end);
	}

	psram_hal_disable_write_through_area(area % PSRAM_WRITE_THROUGH_AREA_PER_PSRAM);
	return BK_OK;
#else
	(void)area;
	(void)enable;
	(void)start;
	(void)end;
	return BK_FAIL;
#endif
}

void psram_hal_set_transfer_mode(uint32_t value)
{
	psram_ll_set_reg4_wrap_config(value);
}

static int psram_hal_APS6408L_init(uint32_t *id)
{
	uint32_t val = 0;

#if (CONFIG_SOC_BK7256XX)
	uint32_t chip_id = bk_get_hardware_chip_id_version();

	if (chip_id == CHIP_VERSION_C)
		psram_hal_set_mode_value(PSRAM_MODE1);
	else
		psram_hal_set_mode_value(PSRAM_MODE3);
#else
	psram_hal_set_mode_value(PSRAM_MODE6);//PSRAM_MODE2
	//psram_hal_set_reg5_value(0x282);
	psram_hal_set_reg5_value(0x380);
#endif

	psram_hal_set_cmd_reset();

	psram_delay(500);

	val = psram_hal_cmd_read(0x00000000);//1 0001 10001101
#if (CONFIG_SOC_BK7256XX)
	// 8 line
	val = ((val >> 8) & 0xFF) | ((val & 0xFF) << 8);
#endif

	if (val == 0 || val != *id)
	{
		return -1;
	}
	else
	{
		*id = val;
	}

	val = psram_hal_get_regb_value();

#if (CONFIG_SOC_BK7256XX)
	val = (val & ~(0x1F << 8)) | (0x4 << 10) | (0x3 << 8);
#else
	val = (val & ~(0x1F)) | (0x4 << 2) | 0x3;
#endif
	psram_hal_cmd_write(0x00000000, val);

	psram_hal_cmd_read(0x00000000);//1 0001 10001101
	psram_hal_cmd_read(0x00000004);

	val = psram_hal_get_regb_value();
#if (CONFIG_SOC_BK7256XX)
	val = (val & ~(0x7 << 13)) | (0x6 << 13);//write latency 110 166Mhz
#else
	val = (val & ~(0x7 << 5)) | (0x6 << 5);
#endif
	psram_hal_cmd_write(0x00000004, val);

	return 0;
}

static int psram_hal_W955D8MKY_5J_init(uint32_t *id)
{
	uint32_t val = 0;
	uint32_t io_drv = 0; /*range [0, 3]*/
	#if (CONFIG_SOC_BK7256XX)
		psram_hal_set_mode_value(PSRAM_MODE4);// mode 4
	#elif (CONFIG_SOC_BK7236XX)
		psram_hal_set_mode_value(PSRAM_MODE8);// mode 8
	#endif
#if (!CONFIG_SOC_BK7256XX)
	psram_hal_set_reg5_value(0x292);
	//psram_hal_set_reg5_value(0x380);	//NOTES:APS PSRAM drive stength needs changed.This type PSRAM needs to be verified.
#endif

	psram_hal_set_cmd_reset();
	psram_delay(500);

	val = psram_hal_cmd_read(0x01000000);
	(VOID *)val;
#if (CONFIG_SOC_BK7256XX)
	// 8 line
	val = ((val >> 8) & 0xFF) | ((val & 0xFF) << 8);
#endif


#if (CONFIG_SOC_BK7256XX)
	val = 0x8F1C | (io_drv << 12);
#else
	val = 0x1C8F | (io_drv << 4);
#endif


#if (CONFIG_SOC_BK7256XX)
	psram_hal_cmd_write(0x01000000, val);
#elif (CONFIG_SOC_BK7236XX)
	psram_hal_cmd_write(0x01000000, 0x1c8f);
#endif
	psram_hal_cmd_read(0x01000000);

	return 0;
}

static int psram_hal_APS128XXO_OB9_init(uint32_t *id)
{
	uint32_t val = 0;
	psram_hal_set_mode_value(PSRAM_MODE7);
#if (!CONFIG_SOC_BK7256XX)
	//psram_hal_set_reg5_value(0x292);
	psram_hal_set_reg5_value(0x380);
#endif
	psram_hal_set_cmd_reset();

	psram_delay(500);

	val = psram_hal_cmd_read(0x00000000);//1 0001 10001101

#if (CONFIG_SOC_BK7256XX)
	// 8 line
	val = ((val >> 8) & 0xFF) | ((val & 0xFF) << 8);
#endif
	if (val == 0 || val != *id)
	{
		return -1;
	}
	else
	{
		*id = val;
	}

	val = psram_hal_get_regb_value();
#if (CONFIG_SOC_BK7256XX)
	val = (val & ~(0x7 << 10)) | (0x4 << 10);
#else
	val = (val & ~(0x7 << 2)) | (0x4 << 2);
#endif
	psram_hal_cmd_write(0x00000000, val);

	psram_hal_cmd_read(0x00000004);
	val = psram_hal_get_regb_value();
#if (CONFIG_SOC_BK7256XX)
	val = (val & ~(0x7 << 13)) | (0x6 << 13);
#else
	val = (val & ~(0x7 << 5)) | (0x6 << 5);
#endif
	psram_hal_cmd_write(0x00000004, val);

	psram_hal_cmd_read(0x00000008);//1 0001 10001101
	val = psram_hal_get_regb_value();
	val |= 0x40;
	psram_hal_cmd_write(0x00000008,val);

	return 0;
}

uint32_t psram_hal_config_init(uint32_t id)
{
	int ret = 0;
	uint32_t val = 0;
	uint32_t type = id;

	psram_hal_set_sf_reset(1);

	/* set psram bypass */
	val = psram_hal_get_reg2_value();
	val |= (0x1 << 1);
	psram_hal_set_reg2_value(val);

	if (id != 0)
	{
		if (id == PSRAM_APS6408L_ID)
		{
			psram_hal_APS6408L_init(&type);
			return type;
		}
		else if (id == PSRAM_APS128XXO_OB9_ID)
		{
			psram_hal_APS128XXO_OB9_init(&type);
			return type;
		}
		else //id == PSRAM_W955D8MKY_5J_ID
		{
			psram_hal_W955D8MKY_5J_init(&type);
			return type;
		}
	}
	else
	{
		type = PSRAM_APS6408L_ID;
		ret = psram_hal_APS6408L_init(&type);
		if (ret == 0)
		{
			return type;
		}

		type = PSRAM_APS128XXO_OB9_ID;
		ret = psram_hal_APS128XXO_OB9_init(&type);
		if (ret == 0)
		{
			return type;
		}

		type = PSRAM_W955D8MKY_5J_ID;
		ret = psram_hal_W955D8MKY_5J_init(&type);

		return type;
	}

}

// config 1: psram power and clk config, need wait clk stable
void psram_hal_power_clk_enable(uint8_t enable)
{
	if (enable)
	{
		psram_delay(500);

		sys_drv_psram_ldo_enable(1);
		bk_delay_us(1000);

		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_PSRAM, PM_POWER_MODULE_STATE_ON);

		// psram bus clk always open
		sys_drv_psram_psram_disckg(1);

		//psram 80M
		psram_hal_set_clk(PSRAM_80M);

		bk_pm_clock_ctrl(CLK_PWR_ID_PSRAM, CLK_PWR_CTRL_PWR_UP);//psram_clk_enable bit19=1
	}
	else
	{
		bk_psram_heap_init_flag_set(false);

		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_PSRAM, PM_POWER_MODULE_STATE_OFF);
		psram_hal_set_sf_reset(0);

		bk_pm_clock_ctrl(CLK_PWR_ID_PSRAM, CLK_PWR_CTRL_PWR_DOWN);//psram_clk_disable

		// power down
		sys_drv_psram_ldo_enable(0);
	}

	psram_delay(3000);
}

// config 2: reset psram and wait psram ready
void psram_hal_reset(void)
{
#if (CONFIG_SOC_BK7256XX)
	psram_hal_set_sf_reset(1);

#if (CONFIG_PSRAM_APS6408L_O)

	int chip_id = bk_get_hardware_chip_id_version();

	if (chip_id == CHIP_VERSION_C)
		psram_hal_set_mode_value(PSRAM_MODE1);
	else
		psram_hal_set_mode_value(PSRAM_MODE3);
#else
	//psram_hal_set_mode_value(PSRAM_MODE5);// mode 5
	psram_hal_set_mode_value(PSRAM_MODE4);// mode 4
#endif
	psram_hal_set_cmd_reset();
#endif //CONFIG_SOC_BK7256XX
}

// config 3: psram config
void psram_hal_config(void)
{
#if (CONFIG_SOC_BK7256XX)
	uint32_t val = 0;

#if (CONFIG_PSRAM_APS6408L_O)

	psram_hal_cmd_read(0x00000000);//1 0001 10001101

	val = psram_hal_get_regb_value();

	val = (val & ~(0x1F << 8)) | (0x4 << 10) | (0x3 << 8);

	psram_hal_cmd_write(0x00000000, val);

	psram_hal_cmd_read(0x00000000);//1 0001 10001101

	psram_hal_cmd_read(0x00000004);

	val = psram_hal_get_regb_value();
	val = (val & ~(0x7 << 13)) | (0x6 << 13);//write latency 110 166Mhz

	psram_hal_cmd_write(0x00000004, val);
#else
	uint32_t io_drv = 0; /*range [0, 3]*/

	val = psram_hal_cmd_read(0x01000000);

	//val = 0x8F1F | (io_drv << 12);// mode 5
	val = 0x8F1C | (io_drv << 12);// mode 4

	psram_hal_cmd_write(0x01000000, val);

	psram_hal_cmd_read(0x01000000);
#endif

#endif //CONFIG_SOC_BK7256XX
}


