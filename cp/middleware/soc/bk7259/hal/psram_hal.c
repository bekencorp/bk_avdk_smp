//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cli.h"
#include <common/bk_include.h>
#include <driver/psram_types.h>
#include "psram_hal.h"
#include "psram_ll_macro_def.h"
#include "bk_pm_internal_api.h"
#include "sys_driver.h"
#include "aon_pmu_ll.h"
#include <driver/psram.h>
#include <modules/chip_support.h>
#if (CONFIG_PSRAM_INTERLEAVE)
#include "sys_ahbp_ll.h"
#endif

extern void bk_delay_us(uint32_t us);

static void psram_delay(volatile uint32_t times)
{
	while(times--);
}

// New functions with psram_id parameter
void psram_hal_set_sf_reset_with_id(psram_id_t psram_id, uint32_t value)
{
	psram_ll_set_sf_reset_value(psram_id, value);
}

void psram_hal_set_cmd_reset_with_id(psram_id_t psram_id)
{
	psram_ll_set_reg8_value(psram_id, 0x4);
}

void psram_hal_cmd_write_with_id(psram_id_t psram_id, uint32_t addr, uint32_t value)
{
	psram_ll_set_write_address(psram_id, addr);
	psram_ll_set_write_data(psram_id, value);
	psram_ll_set_reg8_value(psram_id, 0x1);
	while(psram_ll_get_reg8_value(psram_id) & 0x1);
}

uint32_t psram_hal_cmd_read_with_id(psram_id_t psram_id, uint32_t addr)
{
	uint8_t m = 10, i = 5;

	for (i = 5; i > 0; i--)
	{
		psram_ll_set_write_address(psram_id, addr);
		psram_ll_set_reg8_value(psram_id, 0x2);

		m = 10;

		while(psram_ll_get_reg8_value(psram_id) & 0x2)
		{
			for (int j = 0; j < 5000; j++) {}

			m--;

			if (m == 0)
				break;
		};

		if (m != 0)
		{
			return psram_ll_get_regb_value(psram_id);
		}

		psram_hal_set_sf_reset_with_id(psram_id, 0);
		psram_hal_set_sf_reset_with_id(psram_id, 1);
		psram_hal_set_cmd_reset_with_id(psram_id);
	}

	return 0;
}

// Legacy functions for backward compatibility (use PSRAM_ID_0)
void psram_hal_set_sf_reset(uint32_t value)
{
	psram_hal_set_sf_reset_with_id(PSRAM_ID_0, value);
}

void psram_hal_set_cmd_reset(void)
{
	psram_hal_set_cmd_reset_with_id(PSRAM_ID_0);
}

void psram_hal_cmd_write(uint32_t addr, uint32_t value)
{
	psram_hal_cmd_write_with_id(PSRAM_ID_0, addr, value);
}

uint32_t psram_hal_cmd_read(uint32_t addr)
{
	return psram_hal_cmd_read_with_id(PSRAM_ID_0, addr);
}

void psram_hal_set_clk_with_id(psram_id_t psram_id, psram_clk_t clk)
{
	switch (clk)
	{
		case PSRAM_640M:
			sys_drv_psram_clk_sel_with_id(psram_id, 3);	 // clk sel: 0:320M  1:480M  2:DCO  3:640M
			sys_drv_psram_set_clkdiv_with_id(psram_id, 0); //frq:  F/(1+div)
			break;
		case PSRAM_480M:
			sys_drv_psram_clk_sel_with_id(psram_id, 1);	 // clk sel: 0:320M  1:480M  2:DCO  3:640M
			sys_drv_psram_set_clkdiv_with_id(psram_id, 0); //frq:  F/(1+div)
			break;
		case PSRAM_240M:
			sys_drv_psram_clk_sel_with_id(psram_id, 1);	 // clk sel: 0:320M  1:480M  2:DCO  3:640M
			sys_drv_psram_set_clkdiv_with_id(psram_id, 1); //frq:  F/(1+div)
			break;
		case PSRAM_160M:
			sys_drv_psram_clk_sel_with_id(psram_id, 0);
			sys_drv_psram_set_clkdiv_with_id(psram_id, 1);
			break;
		case PSRAM_120M:
			sys_drv_psram_clk_sel_with_id(psram_id, 1);
			sys_drv_psram_set_clkdiv_with_id(psram_id, 3);
			break;
		case PSRAM_80M:
			sys_drv_psram_clk_sel_with_id(psram_id, 0);
			sys_drv_psram_set_clkdiv_with_id(psram_id, 3);
			break;
		default:
			break;
	}
}

void psram_hal_set_default_clk_with_id(psram_id_t psram_id)
{
	psram_hal_set_clk_with_id(psram_id, PSRAM_480M);
}

void psram_hal_set_clk(psram_clk_t clk)
{
	psram_hal_set_clk_with_id(PSRAM_ID_0, clk);
}

void psram_hal_set_default_clk(void)
{
	psram_hal_set_default_clk_with_id(PSRAM_ID_0);
}

void psram_hal_set_voltage(psram_voltage_t voltage)
{
	uint32_t voltage_sel;

	// BK7259 hardware: vpsramsel supports 1.2V-2.0V, 50mV per step
	// Voltage calculation: voltage = 1.2V + voltage_sel * 50mV
	// Register is 4-bit (0-15), so max voltage is 1.95V (1.2V + 15 * 50mV)
	switch (voltage)
	{
		case PSRAM_OUT_1_95V:
			// 1.95V = 1.2V + 15 * 50mV
			voltage_sel = 15;
			break;
		case PSRAM_OUT_1_90V:
			// 1.90V = 1.2V + 14 * 50mV
			voltage_sel = 14;
			break;
		case PSRAM_OUT_1_85V:
			// 1.85V = 1.2V + 13 * 50mV
			voltage_sel = 13;
			break;
		case PSRAM_OUT_1_80V:
			// 1.80V = 1.2V + 12 * 50mV
			voltage_sel = 12;
			break;
		case PSRAM_OUT_1_50V:
			voltage_sel = 6;
			break;
		case PSRAM_OUT_1_20V:
			voltage_sel = 0;
			break;
		default:
			// Default to 1.8V if unknown voltage
			voltage_sel = 12;
			break;
	}

	// Ensure voltage_sel is within valid range (0-15)
	if (voltage_sel > 15) {
		voltage_sel = 15;
	}

	// Call sys_hal_psram_psldo_vset with voltage_sel as first parameter
	// Note: second parameter (is_add_200mv) is unused in bk7259 implementation
	sys_drv_psram_psldo_vset(voltage_sel, 0);
}

static inline bool is_psram_addr_out_of_range(psram_id_t psram_id, uint32_t addr)
{
#if (CONFIG_PSRAM_WRITE_THROUGH)
	uint32_t data_base = psram_ll_get_data_base(psram_id);
	return ((addr < data_base) || (addr > (data_base + SOC_PSRAM_DATA_SIZE)));
#else
	return false;
#endif
}

// Legacy function for backward compatibility
static inline bool is_psram_addr_out_of_range_legacy(uint32_t addr)
{
	return is_psram_addr_out_of_range(PSRAM_ID_0, addr);
}

static inline bool is_32bytes_aligned(uint32_t addr)
{
	return ((addr & 0x1f) == 0);
}

// New function with psram_id parameter
int psram_hal_set_write_through_with_id(psram_id_t psram_id, psram_write_through_area_t area, uint32_t enable, uint32_t start, uint32_t end)
{
#if (CONFIG_PSRAM_WRITE_THROUGH)

	if (area > PSRAM_WRITE_THROUGH_AREA_COUNT) {
		return BK_ERR_PSRAM_AREA;
	}

	if (enable) {
		if (start >= end) {
			return BK_ERR_PSRAM_ADDR_RELATION;
		}

		if (is_psram_addr_out_of_range(psram_id, start) || is_psram_addr_out_of_range(psram_id, end)) {
			return BK_ERR_PSRAM_ADDR_OUT_OF_RANGE;
		}

		if ((!is_32bytes_aligned(start)) || (!is_32bytes_aligned(end))) {
			return BK_ERR_PSRAM_ADDR_ALIGN;
		}
		psram_ll_set_cover_start(psram_id, area, start >> 5);
		psram_ll_set_cover_stop_enable(psram_id, area, BIT(31) | (end >> 5));
	} else {
		psram_ll_set_cover_stop_enable(psram_id, area, 0);
	}

	return BK_OK;
#else
	return BK_FAIL;
#endif
}

// Legacy function for backward compatibility (use PSRAM_ID_0)
int psram_hal_set_write_through(psram_write_through_area_t area, uint32_t enable, uint32_t start, uint32_t end)
{
	return psram_hal_set_write_through_with_id(PSRAM_ID_0, area, enable, start, end);
}

// New function with psram_id parameter
void psram_hal_set_transfer_mode_with_id(psram_id_t psram_id, uint32_t value)
{
	psram_ll_set_reg4_wrap_config(psram_id, value);
}

// Legacy function for backward compatibility (use PSRAM_ID_0)
void psram_hal_set_transfer_mode(uint32_t value)
{
	psram_hal_set_transfer_mode_with_id(PSRAM_ID_0, value);
}

// New init functions with psram_id parameter
static int psram_hal_APS6408L_init_with_id(psram_id_t psram_id, uint32_t *id)
{
	uint32_t val = 0;

	psram_ll_set_mode_value(psram_id, PSRAM_MODE6);//PSRAM_MODE2
	psram_ll_set_reg5_value(psram_id, 0x14e4);//(0x380);
	psram_hal_set_cmd_reset_with_id(psram_id);

	psram_delay(500);

	val = psram_hal_cmd_read_with_id(psram_id, 0x00000000);//1 0001 10001101
	if (val == 0 || val != *id)
	{
		return -1;
	}
	else
	{
		*id = val;
	}

	val = psram_ll_get_regb_value(psram_id);
	val = (val & ~(0x1F)) | (0x4 << 2) | 0x3;
	psram_hal_cmd_write_with_id(psram_id, 0x00000000, val);

	psram_hal_cmd_read_with_id(psram_id, 0x00000000);//1 0001 10001101
	psram_hal_cmd_read_with_id(psram_id, 0x00000004);

	val = psram_ll_get_regb_value(psram_id);
	val = (val & ~(0x7 << 5)) | (0x6 << 5);
	psram_hal_cmd_write_with_id(psram_id, 0x00000004, val);

	return 0;
}

static int psram_hal_W955D8MKY_5J_init_with_id(psram_id_t psram_id, uint32_t *id)
{
	uint32_t val = 0;
	uint32_t io_drv = 0; /*range [0, 3]*/

	psram_ll_set_mode_value(psram_id, PSRAM_MODE8);// mode 8
	psram_ll_set_reg5_value(psram_id, 0x292);
	psram_hal_set_cmd_reset_with_id(psram_id);
	psram_delay(500);

	val = psram_hal_cmd_read_with_id(psram_id, 0x01000000);
	val = 0x1C8F | (io_drv << 4);
	(VOID *)val;

	psram_hal_cmd_write_with_id(psram_id, 0x01000000, 0x1c8f);
	psram_hal_cmd_read_with_id(psram_id, 0x01000000);

	return 0;
}

static int psram_hal_APS128XXO_OB9_init_with_id(psram_id_t psram_id, uint32_t *id)
{
	uint32_t val = 0;
	psram_ll_set_mode_value(psram_id, PSRAM_MODE7);
	psram_ll_set_reg5_value(psram_id, 0x14e4);
	psram_hal_set_cmd_reset_with_id(psram_id);

	psram_delay(500);

	val = psram_hal_cmd_read_with_id(psram_id, 0x00000000);//1 0001 10001101

	if (val == 0 || val != *id)
	{
		return -1;
	}
	else
	{
		*id = val;
	}

	val = psram_ll_get_regb_value(psram_id);
	val = (val & ~(0x7 << 2)) | (0x4 << 2);
	psram_hal_cmd_write_with_id(psram_id, 0x00000000, val);

	psram_hal_cmd_read_with_id(psram_id, 0x00000004);
	val = psram_ll_get_regb_value(psram_id);
	val = (val & ~(0x7 << 5)) | (0x6 << 5);
	psram_hal_cmd_write_with_id(psram_id, 0x00000004, val);

	psram_hal_cmd_read_with_id(psram_id, 0x00000008);//1 0001 10001101
	val = psram_ll_get_regb_value(psram_id);
	val |= 0x40;
	psram_hal_cmd_write_with_id(psram_id, 0x00000008, val);

	return 0;
}

static int psram_hal_SCB18X128XX_OAF_init_with_id(psram_id_t psram_id, uint32_t *id)
{
	uint32_t val = 0;
	psram_ll_set_mode_value(psram_id, PSRAM_MODE9);
	psram_ll_set_reg5_value(psram_id, (4 << 0) + (4 << 3) + (2 << 6) + (0 << 9) + (0 << 12));
	psram_hal_set_cmd_reset_with_id(psram_id);

	psram_delay(500);

	val = psram_hal_cmd_read_with_id(psram_id, 0x00000000);//1 0001 10001101
	if (val == 0 || val != *id)
	{
		return -1;
	}
	else
	{
		*id = val;
	}

	psram_hal_cmd_read_with_id(psram_id, 0x00000000);//1 0001 10001101
	val = psram_ll_get_regb_value(psram_id);
	val = (val & ~(0x7 << 2)) | (0x0 << 2) | (0x0 << 5);//320M
	//val = (val & ~(0x7 << 2)) | (0x6 << 2) | (0x0 << 5);//240M
	psram_hal_cmd_write_with_id(psram_id, 0x00000000, val);

	psram_hal_cmd_read_with_id(psram_id, 0x00000004);//1 0001 10001101
	val = psram_ll_get_regb_value(psram_id);
	val = (val & ~(0x7 << 5)) | (0x4 << 5);//320mhz write latency
	//val = (val & ~(0x7 << 5)) | (0x3 << 5);//240mhz write latency
	psram_hal_cmd_write_with_id(psram_id, 0x00000004, val);

	psram_hal_cmd_read_with_id(psram_id, 0x00000008);//1 0001 10001101
	val = psram_ll_get_regb_value(psram_id);
	val = 0x3 << 0 | 0x3 << 5 ;//x16 1k hword wrap en high freq(320Mhz)
	//val = 0x3 << 0 | 0x2 << 5 ;//x16 1k hword wrap not en high freq(240Mhz)
	psram_hal_cmd_write_with_id(psram_id, 0x00000008, val);

	return 0;
}

// New function with psram_id parameter
uint32_t psram_hal_config_init_with_id(psram_id_t psram_id, uint32_t id)
{
	int ret = 0;
	uint32_t val = 0;
	uint32_t type = id;

	psram_hal_set_sf_reset_with_id(psram_id, 1);

	/* set psram bypass */
	val = psram_ll_get_reg2_value(psram_id);
	val |= (0x1 << 1);
	psram_ll_set_reg2_value(psram_id, val);

	if (id != 0)
	{
		if (id == PSRAM_APS6408L_ID)
		{
			psram_hal_APS6408L_init_with_id(psram_id, &type);
			return type;
		}
		else if (id == PSRAM_APS128XXO_OB9_ID)
		{
			psram_hal_APS128XXO_OB9_init_with_id(psram_id, &type);
			return type;
		}
		else if (id == PSRAM_SCB18X128XX_OAF_ID)
		{
			psram_hal_SCB18X128XX_OAF_init_with_id(psram_id, &type);
			return type;
		}
		else //id == PSRAM_W955D8MKY_5J_ID
		{
			psram_hal_W955D8MKY_5J_init_with_id(psram_id, &type);
			return type;
		}
	}
	else
	{
		type = PSRAM_APS6408L_ID;
		ret = psram_hal_APS6408L_init_with_id(psram_id, &type);
		if (ret == 0)
		{
			return type;
		}

		type = PSRAM_APS128XXO_OB9_ID;
		ret = psram_hal_APS128XXO_OB9_init_with_id(psram_id, &type);
		if (ret == 0)
		{
			return type;
		}

		type = PSRAM_SCB18X128XX_OAF_ID;
		ret = psram_hal_SCB18X128XX_OAF_init_with_id(psram_id, &type);
		if (ret == 0)
		{
			return type;
		}

		type = PSRAM_W955D8MKY_5J_ID;
		ret = psram_hal_W955D8MKY_5J_init_with_id(psram_id, &type);

		return type;
	}
}

// Legacy init functions for backward compatibility (use PSRAM_ID_0)
static int psram_hal_APS6408L_init(uint32_t *id)
{
	return psram_hal_APS6408L_init_with_id(PSRAM_ID_0, id);
}

static int psram_hal_W955D8MKY_5J_init(uint32_t *id)
{
	return psram_hal_W955D8MKY_5J_init_with_id(PSRAM_ID_0, id);
}

static int psram_hal_APS128XXO_OB9_init(uint32_t *id)
{
	return psram_hal_APS128XXO_OB9_init_with_id(PSRAM_ID_0, id);
}

static int psram_hal_SCB18X128XX_OAF_init(uint32_t *id)
{
	return psram_hal_SCB18X128XX_OAF_init_with_id(PSRAM_ID_0, id);
}
uint32_t psram_hal_config_init(uint32_t id)
{
	return psram_hal_config_init_with_id(PSRAM_ID_0, id);
}

// config 1: psram power and clk config, need wait clk stable
void psram_hal_power_clk_enable(uint8_t enable)
{
	if (enable)
	{
		psram_delay(500);

		sys_drv_psram_ldo_enable(1);
		bk_delay_us(1000);


		//bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_PSRAM, PM_POWER_MODULE_STATE_ON);


		// psram bus clk always open
		sys_drv_psram_psram0_disckg(1);
		sys_drv_psram_psram1_disckg(1);

		//psram 80M
		psram_hal_set_clk_with_id(PSRAM_ID_0, PSRAM_80M);
		psram_hal_set_clk_with_id(PSRAM_ID_1, PSRAM_80M);

		sys_drv_dev_clk_pwr_up(CLK_PWR_ID_PSRAM0, CLK_PWR_CTRL_PWR_UP);
		sys_drv_dev_clk_pwr_up(CLK_PWR_ID_PSRAM1, CLK_PWR_CTRL_PWR_UP);
	}
	else
	{
		bk_psram_heap_init_flag_set(false);

		//bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_PSRAM, PM_POWER_MODULE_STATE_OFF);
		psram_hal_set_sf_reset_with_id(PSRAM_ID_0, 0);
		psram_hal_set_sf_reset_with_id(PSRAM_ID_1, 0);

		sys_drv_dev_clk_pwr_up(CLK_PWR_ID_PSRAM0, CLK_PWR_CTRL_PWR_DOWN);
		sys_drv_dev_clk_pwr_up(CLK_PWR_ID_PSRAM1, CLK_PWR_CTRL_PWR_DOWN);

		// power down
		sys_drv_psram_ldo_enable(0);
	}

	psram_delay(3000);
}

// config 2: reset psram and wait psram ready
void psram_hal_reset(void)
{
}

// config 3: psram config
void psram_hal_config(void)
{
}

void psram_hal_set_interleave_config(uint32_t step)
{
#if (CONFIG_PSRAM_INTERLEAVE)
	sys_ahbp_ll_set_reg7_psram_inv_config(step);
#else
	(void)step;
#endif
}

