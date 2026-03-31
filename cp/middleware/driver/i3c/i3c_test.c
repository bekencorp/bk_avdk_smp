/**
 * @file i3c_test.c
 * @brief I3C CLI test: config API + data transfer (poll/interrupt)
 *
 * Commands: i3c mst [sdr|hdr] [clk|500k|1M|4M|10M|12M] [tx|rx] [int|poll]
 *   With default f_core≈98.304 MHz, "10M" and "12M" both truncate to ~12.288 MHz SCL (see I3C_MST_SDR_TARGET_HZ_12M288).
 *   Near-max SDR (~12 MHz class) needs short traces and clean SI; long jumpers often pass 2M/4M but fail after ENUM_MAX→final bump.
 *       i3c slv <0|1|2> [sdr|hdr] [tx|rx] [int|poll]
 * Order: start slave first, then master
 */
#if defined(CONFIG_I3C_TEST)

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdio.h>
#include "cli.h"
#include "cli_section.h"
#include "i3c.h"
#include "gpio_driver.h"

#ifndef I3C_TEST_GPIO_SCL
#define I3C_TEST_GPIO_SCL  GPIO_32
#define I3C_TEST_GPIO_SDA  GPIO_33
#define I3C_TEST_GPIO_PURN GPIO_34
#endif

#ifndef I3C_TEST_HDR_PAYLOAD_BYTES
#define I3C_TEST_HDR_PAYLOAD_BYTES  128u
#endif
#define I3C_PARAM_MAX      5
#define I3C_PARAM_INT_MODE 4

#define I3C_TEST_DEV_ADDR    0x09u
#define I3C_TEST_TIMEOUT_MS  20000u

static const i3c_platform_config_t s_platform = {
	.gpio_scl  = I3C_TEST_GPIO_SCL,
	.gpio_sda  = I3C_TEST_GPIO_SDA,
	.gpio_purn = I3C_TEST_GPIO_PURN,
};

uint32_t s_i3c_mst_freq;

static void test_i3c_run(uint8_t *p)
{
	uint8_t clk_div = (p[2] != 0) ? p[2] : 1;
	uint32_t use_freq = s_i3c_mst_freq;
	s_i3c_mst_freq = 0;

	i3c_master_config_t mst_cfg = {
		.platform   = &s_platform,
		.mode       = I3C_MST_MODE_SDR,
		.clk_div    = clk_div,
		.target_hz  = use_freq,
		.mwl_bytes  = 0,
		.mrl_bytes  = 0,
	};
	i3c_slave_config_t slv_cfg = {
		.platform   = &s_platform,
		.slv_device = 0,
		.mode       = I3C_SLV_MODE_SDR,
		.max_mwl    = 0,
		.max_mrl    = 0,
	};

	if (p[0] == 3) {
		I3C_MST_LOGI("i3c: mst sdr %s=%u\r\n", use_freq ? "freq" : "clk_div", use_freq ? use_freq : (unsigned)clk_div);
		if (bk_i3c_master_init(&mst_cfg) != BK_OK) {
			I3C_MST_LOGE("i3c: master init failed\r\n");
			goto done;
		}
		i3c_xfer_mode_t xfer = (p[I3C_PARAM_INT_MODE] != 0u) ? I3C_XFER_INT : I3C_XFER_POLL;

		if (p[3] == 0) {
			uint8_t tx[256];
			for (uint32_t i = 0; i < sizeof(tx); i++)
				tx[i] = (uint8_t)(i + 1);
			bk_i3c_master_write(I3C_TEST_DEV_ADDR, tx, sizeof(tx), I3C_TEST_TIMEOUT_MS);
		} else {
			uint8_t rx[128];
			uint32_t recv_len;
			bk_i3c_master_read(I3C_TEST_DEV_ADDR, rx, sizeof(rx), &recv_len, I3C_TEST_TIMEOUT_MS, xfer);
			I3C_MST_LOGI("i3c mst sdr rx: %u bytes\r\n", (unsigned)recv_len);
			for (unsigned i = 0; i < recv_len; i += 8u) {
				char line[64];
				int n = snprintf(line, sizeof(line), "  %03u:", i);
				for (unsigned j = 0; j < 8u && (i + j) < recv_len; j++)
					n += snprintf(line + n, sizeof(line) - (unsigned)n, " %02x", (unsigned)rx[i + j]);
				snprintf(line + n, sizeof(line) - (unsigned)n, "\r\n");
				I3C_MST_LOGI("%s", line);
			}
		}
	} else if (p[0] >= 4 && p[0] <= 6) {
		uint8_t id = (uint8_t)(p[0] - 4);
		slv_cfg.slv_device = id;
		bk_i3c_slave_init(&slv_cfg);
		i3c_xfer_mode_t xfer = (p[I3C_PARAM_INT_MODE] != 0u) ? I3C_XFER_INT : I3C_XFER_POLL;

		if (p[3] == 0) {
			uint8_t tx[128];
			for (uint32_t i = 0; i < sizeof(tx); i++)
				tx[i] = (uint8_t)(i + 1);
			bk_i3c_slave_write(tx, sizeof(tx), 0u, xfer);
		} else {
			uint8_t rx[256];
			uint32_t recv_len;
			bk_i3c_slave_read(rx, sizeof(rx), &recv_len, 0u, 0u, xfer);
			I3C_SLV_LOGI("i3c slv sdr rx: %u bytes\r\n", (unsigned)recv_len);
			for (unsigned i = 0; i < recv_len; i += 8u) {
				char line[64];
				int n = snprintf(line, sizeof(line), "  %03u:", i);
				for (unsigned j = 0; j < 8u && (i + j) < recv_len; j++)
					n += snprintf(line + n, sizeof(line) - (unsigned)n, " %02x", (unsigned)rx[i + j]);
				snprintf(line + n, sizeof(line) - (unsigned)n, "\r\n");
				I3C_SLV_LOGI("%s", line);
			}
		}
	} else if (p[0] == 7) {
		I3C_MST_LOGI("i3c: mst hdr %s=%u\r\n", use_freq ? "freq" : "clk_div", use_freq ? use_freq : (unsigned)clk_div);
		mst_cfg.mode = I3C_MST_MODE_HDR;
		if (bk_i3c_master_init(&mst_cfg) != BK_OK) {
			I3C_MST_LOGE("i3c: hdr master init failed\r\n");
			goto done;
		}
		i3c_xfer_mode_t xfer = (p[I3C_PARAM_INT_MODE] != 0u) ? I3C_XFER_INT : I3C_XFER_POLL;

		if (p[3] == 0) {
			uint8_t tx[(unsigned)I3C_TEST_HDR_PAYLOAD_BYTES];
			for (uint32_t i = 0; i < sizeof(tx); i++)
				tx[i] = (uint8_t)(i + 1);
			bk_i3c_master_write(I3C_TEST_DEV_ADDR, tx, sizeof(tx), I3C_TEST_TIMEOUT_MS);
		} else {
			uint8_t rx[256];
			uint32_t recv_len;
			bk_i3c_master_read(I3C_TEST_DEV_ADDR, rx, sizeof(rx), &recv_len, I3C_TEST_TIMEOUT_MS, xfer);
			I3C_MST_LOGI("i3c mst hdr rx: %u bytes\r\n", (unsigned)recv_len);
			for (unsigned i = 0; i < recv_len; i += 8u) {
				char line[64];
				int n = snprintf(line, sizeof(line), "  %03u:", i);
				for (unsigned j = 0; j < 8u && (i + j) < recv_len; j++)
					n += snprintf(line + n, sizeof(line) - (unsigned)n, " %02x", (unsigned)rx[i + j]);
				snprintf(line + n, sizeof(line) - (unsigned)n, "\r\n");
				I3C_MST_LOGI("%s", line);
			}
		}
	} else if (p[0] >= 8 && p[0] <= 10) {
		uint8_t id = (uint8_t)(p[0] - 8);
		slv_cfg.slv_device = id;
		slv_cfg.mode = I3C_SLV_MODE_HDR;
		bk_i3c_slave_init(&slv_cfg);
		i3c_xfer_mode_t xfer = (p[I3C_PARAM_INT_MODE] != 0u) ? I3C_XFER_INT : I3C_XFER_POLL;

		if (p[3] == 0u) {
			uint8_t tx[(unsigned)I3C_TEST_HDR_PAYLOAD_BYTES];
			for (uint32_t i = 0; i < sizeof(tx); i++)
				tx[i] = (uint8_t)(i + 1);
			bk_i3c_slave_write(tx, sizeof(tx), 0u, xfer);
		} else {
			uint8_t rx[128];
			uint32_t recv_len;
			bk_i3c_slave_read(rx, sizeof(rx), &recv_len, I3C_TEST_HDR_PAYLOAD_BYTES, 0u, xfer);
			I3C_SLV_LOGI("i3c slv hdr rx: %u bytes\r\n", (unsigned)recv_len);
			for (unsigned i = 0; i < recv_len; i += 8u) {
				char line[64];
				int n = snprintf(line, sizeof(line), "  %03u:", i);
				for (unsigned j = 0; j < 8u && (i + j) < recv_len; j++)
					n += snprintf(line + n, sizeof(line) - (unsigned)n, " %02x", (unsigned)rx[i + j]);
				snprintf(line + n, sizeof(line) - (unsigned)n, "\r\n");
				I3C_SLV_LOGI("%s", line);
			}
		}
	}

done:
	i3c_platform_deinit();
	I3C_TEST_LOGI("i3c test end.\r\n");
}

static void cli_i3c_help(char *pcWriteBuffer, int xWriteBufferLen)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;
	I3C_MST_LOGI("i3c mst [sdr|hdr] [clk|500k|1M|4M|12M] [tx|rx] [int|poll]\r\n");
	I3C_MST_LOGI("i3c devs  (master SDR 1M, list enumerated devices)\r\n");
	I3C_SLV_LOGI("i3c slv <0|1|2> [sdr|hdr] [tx|rx] [int|poll]\r\n");
}

static void cli_i3c_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint8_t p[I3C_PARAM_MAX];
	unsigned long val;

	if (argc < 2) {
		cli_i3c_help(pcWriteBuffer, xWriteBufferLen);
		return;
	}

	if (os_strcmp(argv[1], "devs") == 0) {
		i3c_master_config_t mst_cfg = {
			.platform = &s_platform,
			.mode = I3C_MST_MODE_SDR,
			.clk_div = 0,
			.target_hz = 1000000u,
			.mwl_bytes = 0,
			.mrl_bytes = 0,
			.core_clk_src = I3C_CORE_CLK_SRC_APLL,
			.core_hz = 0,
		};
		i3c_platform_init(&s_platform);
		if (bk_i3c_master_init(&mst_cfg) != BK_OK)
			I3C_MST_LOGE("i3c devs: master init failed\r\n");
		bk_i3c_master_deinit();
		i3c_platform_deinit();
		return;
	}

	os_memset(p, 0, sizeof(p));
	p[2] = 1;

	if (os_strcmp(argv[1], "mst") == 0) {
		if (argc < 3) {
			cli_i3c_help(pcWriteBuffer, xWriteBufferLen);
			return;
		}
		if (os_strcmp(argv[2], "sdr") == 0)
			p[0] = 3;
		else if (os_strcmp(argv[2], "hdr") == 0)
			p[0] = 7;
		else {
			I3C_MST_LOGI("i3c mst: use sdr or hdr\r\n");
			return;
		}
		if (argc >= 4) {
			const char *s = argv[3];
			unsigned n = 0;
			val = 0;
			while (s[n] >= '0' && s[n] <= '9')
				val = val * 10u + (unsigned long)(s[n++] - '0');
			if (s[n] == 'k' || s[n] == 'K') {
				s_i3c_mst_freq = (uint32_t)(val * 1000u);
				p[2] = 0;
			} else if (s[n] == 'M' || s[n] == 'm') {
				s_i3c_mst_freq = (uint32_t)(val * 1000000u);
				p[2] = 0;
			} else if (s[n] == '\0' && val <= 0xffu) {
				p[2] = (uint8_t)val;
			}
		}
		if (argc >= 5)
			p[3] = (os_strcmp(argv[4], "rx") == 0) ? 1u : 0u;
		if (argc >= 6 && os_strcmp(argv[5], "int") == 0)
			p[I3C_PARAM_INT_MODE] = 1u;
	} else if (os_strcmp(argv[1], "slv") == 0) {
		if (argc < 3) {
			I3C_SLV_LOGI("i3c slv <0|1|2> [sdr|hdr] [tx|rx] [int|poll]\r\n");
			return;
		}
		val = os_strtoul(argv[2], NULL, 10);
		if (val > 2) {
			I3C_TEST_LOGI("slv id must be 0, 1 or 2\r\n");
			return;
		}
		unsigned arg_tx_rx = 3, arg_int = 4;  /* argv indices */
		if (argc >= 4 && os_strcmp(argv[3], "hdr") == 0) {
			p[0] = (uint8_t)(8 + val);
			arg_tx_rx = 4;
			arg_int = 5;
		} else if (argc >= 4 && os_strcmp(argv[3], "sdr") == 0) {
			p[0] = (uint8_t)(4 + val);
			arg_tx_rx = 4;
			arg_int = 5;
		} else {
			p[0] = (uint8_t)(4 + val);  /* default sdr */
		}
		if (argc >= arg_tx_rx + 1) {
			if (os_strcmp(argv[arg_tx_rx], "rx") == 0) p[3] = 1u;
			else if (os_strcmp(argv[arg_tx_rx], "tx") == 0) p[3] = 0u;
		}
		if (argc >= arg_int + 1 && os_strcmp(argv[arg_int], "int") == 0)
			p[I3C_PARAM_INT_MODE] = 1u;
	} else {
		cli_i3c_help(pcWriteBuffer, xWriteBufferLen);
		return;
	}

	I3C_TEST_LOGI("p[0]=%d p[2]=%d p[3]=%d %s\r\n", p[0], p[2], p[3], p[I3C_PARAM_INT_MODE] ? "int" : "poll");
	test_i3c_run(p);
}

#if CONFIG_DRV_CLI_SUPPORT
DRV_CLI_CMD_EXPORT static const struct cli_command s_i3c_commands[] = {
	{"i3c", "i3c devs | i3c mst [sdr|hdr] [args] | i3c slv <0|1|2> [sdr|hdr] [tx|rx] [int|poll]", cli_i3c_cmd},
};
#endif

#endif /* CONFIG_I3C_TEST */
