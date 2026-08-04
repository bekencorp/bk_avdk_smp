// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <driver/i2c.h>
#include "cli.h"
#include "components/shell_task.h"

#include <driver/gpio.h>
#include "gpio_driver.h"
#include "sys_driver.h"

#define EEPROM_DEV_ADDR          0x50
#define EEPROM_MEM_ADDR          0x10
#define I2C_SLAVE_ADDR           0x42
#define I2C_WRITE_WAIT_MAX_MS    (500)
#define I2C_READ_WAIT_MAX_MS     (500)
#define CAMERA_DEV_ADDR          (0x21)

/* ZD24C128A (C918271_EEPROM_ZD24C128A-SSGMB): 256 pages x 64 bytes = 16KB; Self-timed Write Cycle Within 5ms Max.
 * I2C slave, 2-wire SCL/SDA; A2=A1=A0=GND -> 7bit addr 0x50; 16bit word address. */
#define ZD24C128_DEV_ADDR        0x50
#define ZD24C128_PAGE_SIZE       64
#define ZD24C128_WRITE_CYCLE_MS  5
#define ZD24C128_SIZE            (256 * 64)
#define BUF_CTOR()              do{\
        if(argc > 3){\
                data_length = os_strtoul(argv[3], NULL, 10);\
        }\
        data_buf = os_zalloc(data_length);\
        if(NULL == data_buf)\
        {\
                CLI_LOGE("malloc fail\r\n");\
                return;\
        }\
}while(0)

#define BUF_DTOR()              do{\
        if(data_buf){\
                os_free(data_buf);\
                data_buf = NULL;\
        }\
}while(0)


uint8_t sensor_gc0328c_init_talbe_test1[][2] =
{
{0xF1, 0x00},
{0xF2, 0x00},
{0xFE, 0x00},
{0x4F, 0x00},
{0x42, 0x00},
{0x77, 0x5A},
{0x78, 0x40},
{0x79, 0x56},
{0xFE, 0x00},
{0x0D, 0x01},
{0x0E, 0xE8},//480 + 8
{0x0F, 0x02},
{0x81, 0x58},
{0x82, 0x98},
{0x83, 0x60},
{0x84, 0x58},
};

static void gpio_debug(uint32_t gpio_id)
{
#if CONFIG_USR_GPIO_CFG_EN
	BK_LOG_ON_ERR(bk_gpio_disable_input(gpio_id));
	BK_LOG_ON_ERR(bk_gpio_enable_output(gpio_id));
#endif
	bk_gpio_set_output_high(gpio_id);
	bk_gpio_set_output_low(gpio_id);
	bk_gpio_set_output_high(gpio_id);
	bk_gpio_set_output_low(gpio_id);
}

/* ---- lightweight self-check helpers so test cases print machine-parseable PASS/FAIL ---- */
#define I2C_TC_PASS(name, fmt, ...)  CLI_LOGI("[I2C][PASS] %s: " fmt "\r\n", name, ##__VA_ARGS__)
#define I2C_TC_FAIL(name, fmt, ...)  CLI_LOGE("[I2C][FAIL] %s: " fmt "\r\n", name, ##__VA_ARGS__)

/* Read back `len` bytes from (dev_addr, mem_addr) and compare against `expect`.
 * Returns the number of mismatched bytes (0 == full match). `ret_out` (optional)
 * receives the driver return code; on read failure the whole range counts as mismatch. */
static uint32_t i2c_mem_readback_cmp(uint32_t i2c_id, uint32_t dev_addr, uint32_t mem_addr,
				     i2c_mem_addr_size_t addr_size, const uint8_t *expect,
				     uint32_t len, bk_err_t *ret_out)
{
	uint8_t *rd = os_zalloc(len);
	if (!rd) {
		if (ret_out)
			*ret_out = BK_FAIL;
		return len;
	}

	i2c_mem_param_t p = {0};
	p.dev_addr = dev_addr;
	p.mem_addr = mem_addr;
	p.mem_addr_size = addr_size;
	p.data = rd;
	p.data_size = len;
	p.timeout_ms = I2C_READ_WAIT_MAX_MS;

	bk_err_t ret = bk_i2c_memory_read(i2c_id, &p);
	if (ret_out)
		*ret_out = ret;
	if (ret != BK_OK) {
		os_free(rd);
		return len;
	}

	uint32_t err_cnt = 0;
	for (uint32_t i = 0; i < len; i++) {
		if (rd[i] != expect[i]) {
			if (err_cnt < 8)
				CLI_LOGE("  mismatch [%u]: expect=0x%02x read=0x%02x\r\n", (unsigned)i, expect[i], rd[i]);
			err_cnt++;
		}
	}
	os_free(rd);
	return err_cnt;
}

/* ============================================================================
 * Single-board I2C0<->I2C1 loopback master/slave co-test + exception recovery.
 *
 * WIRING: jumper the two controllers together on the board:
 *   I2C0.SDA <-> I2C1.SDA , I2C0.SCL <-> I2C1.SCL , plus bus pull-ups.
 * BK7259 uses an IO-matrix so the physical GPIOs are board-configurable; the
 * driver maps them by function on bk_i2c_init().
 *
 * Master runs in the CLI task; the slave runs in a worker thread. Because both
 * slave APIs block, we hand-shake with two semaphores (armed/done) so the slave
 * is guaranteed to be armed before the master starts (replaces ESP's UART
 * unity_send_signal/wait_for_signal cross-board sync).
 * ============================================================================ */
#define I2C_LB_MST_ID          I2C_ID_0
#define I2C_LB_SLV_ID          I2C_ID_1
#define I2C_LB_SLV_ADDR        0x42
#define I2C_LB_BAUD            400000
#define I2C_LB_DEF_LEN         16
#define I2C_LB_TIMEOUT_MS      2000
/* smaller number == higher actual priority; must beat the CLI/master task
 * (SHELL_TASK_PRIORITY=4) so the slave keeps its FIFO fed while the master clocks. */
#define I2C_LB_SLV_TASK_PRIO   3
#define I2C_LB_SLV_TASK_STACK  4096
#define I2C_LB_ARM_MARGIN_MS   8       /* let slave HW finish arming after 'armed' signal */

/* Optional HW-assisted stuck-bus test: needs a spare GPIO physically wired to
 * the SDA line so it can be pulled low. Disabled by default (in-chip simulation
 * is not reliable: remapping a pin to the I2C function drops any GPIO drive). */
#ifndef I2C_LB_STUCK_BUS_EN
#define I2C_LB_STUCK_BUS_EN    0
#endif
#ifndef I2C_LB_STUCK_GPIO
#define I2C_LB_STUCK_GPIO      0       /* board-specific; set when I2C_LB_STUCK_BUS_EN=1 */
#endif

enum { I2C_LB_SLV_READ = 0, I2C_LB_SLV_WRITE };

typedef struct {
	beken_semaphore_t armed;   /* slave -> master: slave is about to block in its API */
	beken_semaphore_t done;    /* slave -> master: slave API returned */
	uint8_t *buf;              /* shared: slave rx target (READ) or slave tx source (WRITE) */
	uint32_t len;
	int mode;                  /* I2C_LB_SLV_READ / I2C_LB_SLV_WRITE */
	bk_err_t slv_ret;          /* slave API return code */
} i2c_lb_ctx_t;

static i2c_lb_ctx_t s_lb_ctx;
static beken_thread_t s_lb_slv_thread;
static int s_lb_saved_log_level = -1;   /* quiet DEBUG spam during a run, restored on teardown */

static void i2c_lb_slave_task(beken_thread_arg_t arg)
{
	i2c_lb_ctx_t *c = (i2c_lb_ctx_t *)arg;

	if (c->mode == I2C_LB_SLV_WRITE) {
		for (uint32_t i = 0; i < c->len; i++)
			c->buf[i] = (i + 1) & 0xff;         /* ramp pattern for master to read back */
		rtos_set_semaphore(&c->armed);
		c->slv_ret = bk_i2c_slave_write(I2C_LB_SLV_ID, c->buf, c->len, I2C_LB_TIMEOUT_MS);
	} else {
		rtos_set_semaphore(&c->armed);
		c->slv_ret = bk_i2c_slave_read(I2C_LB_SLV_ID, c->buf, c->len, I2C_LB_TIMEOUT_MS);
	}

	rtos_set_semaphore(&c->done);
	rtos_delete_thread(NULL);
}

/* Init driver + both controllers (I2C0 master role, I2C1 slave role). */
static bk_err_t i2c_lb_setup(void)
{
	/* Silence DEBUG-level driver/OS spam during the co-test: high-rate UART
	 * logging jitters scheduling and can starve the slave FIFO. Keep INFO so
	 * PASS/FAIL and mismatch lines still print; restored in i2c_lb_teardown(). */
	if (s_lb_saved_log_level < 0) {
		s_lb_saved_log_level = shell_get_log_level();
		shell_set_log_level(BK_LOG_INFO);
	}

	bk_err_t ret = bk_i2c_driver_init();
	if (ret != BK_OK)
		return ret;

	i2c_config_t mcfg = {0};
	mcfg.baud_rate = I2C_LB_BAUD;
	mcfg.addr_mode = I2C_ADDR_MODE_7BIT;
	mcfg.slave_addr = 0x00;
	ret = bk_i2c_init(I2C_LB_MST_ID, &mcfg);
	if (ret != BK_OK)
		return ret;

	i2c_config_t scfg = {0};
	scfg.baud_rate = I2C_LB_BAUD;
	scfg.addr_mode = I2C_ADDR_MODE_7BIT;
	scfg.slave_addr = I2C_LB_SLV_ADDR;
	return bk_i2c_init(I2C_LB_SLV_ID, &scfg);
}

static void i2c_lb_teardown(void)
{
	bk_i2c_deinit(I2C_LB_MST_ID);
	bk_i2c_deinit(I2C_LB_SLV_ID);

	if (s_lb_saved_log_level >= 0) {
		shell_set_log_level(s_lb_saved_log_level);
		s_lb_saved_log_level = -1;
	}
}

/* Run one master<->slave transfer. Returns the driver error (master error takes
 * priority, else slave error); *mismatch_out gets the count of data mismatches. */
static bk_err_t i2c_lb_run_once(uint32_t len, int slv_mode, uint32_t *mismatch_out)
{
	uint32_t mismatch = 0;
	bk_err_t mret = BK_OK;

	uint8_t *mbuf = os_zalloc(len);
	if (!mbuf) {
		if (mismatch_out)
			*mismatch_out = len;
		return BK_FAIL;
	}
	s_lb_ctx.buf = os_zalloc(len);
	if (!s_lb_ctx.buf) {
		os_free(mbuf);
		if (mismatch_out)
			*mismatch_out = len;
		return BK_FAIL;
	}
	s_lb_ctx.len = len;
	s_lb_ctx.mode = slv_mode;
	s_lb_ctx.slv_ret = BK_OK;
	rtos_init_semaphore(&s_lb_ctx.armed, 1);
	rtos_init_semaphore(&s_lb_ctx.done, 1);

	if (rtos_create_thread(&s_lb_slv_thread, I2C_LB_SLV_TASK_PRIO, "i2c_lb_slv",
			       i2c_lb_slave_task, I2C_LB_SLV_TASK_STACK, (beken_thread_arg_t)&s_lb_ctx) != kNoErr) {
		rtos_deinit_semaphore(&s_lb_ctx.armed);
		rtos_deinit_semaphore(&s_lb_ctx.done);
		os_free(s_lb_ctx.buf);
		s_lb_ctx.buf = NULL;
		os_free(mbuf);
		if (mismatch_out)
			*mismatch_out = len;
		return BK_FAIL;
	}

	rtos_get_semaphore(&s_lb_ctx.armed, I2C_LB_TIMEOUT_MS);
	rtos_delay_milliseconds(I2C_LB_ARM_MARGIN_MS);

	if (slv_mode == I2C_LB_SLV_READ) {
		for (uint32_t i = 0; i < len; i++)
			mbuf[i] = (i + 1) & 0xff;
		mret = bk_i2c_master_write(I2C_LB_MST_ID, I2C_LB_SLV_ADDR, mbuf, len, I2C_LB_TIMEOUT_MS);
	} else {
		mret = bk_i2c_master_read(I2C_LB_MST_ID, I2C_LB_SLV_ADDR, mbuf, len, I2C_LB_TIMEOUT_MS);
	}

	rtos_get_semaphore(&s_lb_ctx.done, I2C_LB_TIMEOUT_MS + 100);

	/* master always knows the ramp; compare whichever buffer holds the received data */
	const uint8_t *got = (slv_mode == I2C_LB_SLV_READ) ? s_lb_ctx.buf : mbuf;
	for (uint32_t i = 0; i < len; i++) {
		uint8_t exp = (i + 1) & 0xff;
		if (got[i] != exp) {
			if (mismatch < 8)
				CLI_LOGE("  mismatch [%u]: expect=0x%02x got=0x%02x\r\n", (unsigned)i, exp, got[i]);
			mismatch++;
		}
	}

	bk_err_t sret = s_lb_ctx.slv_ret;

	rtos_deinit_semaphore(&s_lb_ctx.armed);
	rtos_deinit_semaphore(&s_lb_ctx.done);
	os_free(s_lb_ctx.buf);
	s_lb_ctx.buf = NULL;
	os_free(mbuf);

	if (mismatch_out)
		*mismatch_out = mismatch;
	if (mret != BK_OK)
		return mret;
	return sret;
}

/* Report a single loopback run as one PASS/FAIL line. Returns 1 on failure. */
static int i2c_lb_eval(const char *name, bk_err_t ret, uint32_t mismatch, uint32_t len)
{
	if (ret != BK_OK) {
		I2C_TC_FAIL(name, "transfer err=%d (len=%u)", ret, (unsigned)len);
		return 1;
	}
	if (mismatch) {
		I2C_TC_FAIL(name, "%u/%u bytes mismatch", (unsigned)mismatch, (unsigned)len);
		return 1;
	}
	I2C_TC_PASS(name, "len=%u verified", (unsigned)len);
	return 0;
}

/* ---- exception-recovery building blocks (assume controllers already set up) ----
 * Each prints per-step PASS/FAIL and returns a failure count. */
static int i2c_rc_nack_then_ok(uint32_t len)
{
	int errs = 0;
	uint8_t d[2] = {0xA5, 0x5A};
	bk_err_t r1 = bk_i2c_master_write(I2C_LB_MST_ID, 0x7E, d, sizeof(d), 100);
	if (r1 == BK_OK) {
		errs++;
		I2C_TC_FAIL("recover.nack_then_ok", "step1 unexpected ACK from absent 0x7E");
	} else {
		I2C_TC_PASS("recover.nack_then_ok", "step1 absent dev err=%d as expected", r1);
	}
	uint32_t mm = 0;
	bk_err_t r2 = i2c_lb_run_once(len, I2C_LB_SLV_READ, &mm);
	errs += i2c_lb_eval("recover.nack_then_ok.step2", r2, mm, len);
	return errs;
}

static int i2c_rc_short_timeout_recover(uint32_t len)
{
	int errs = 0;
	uint8_t d[2] = {0x12, 0x34};
	/* very short timeout against a non-ACKing address exercises the quick timeout path */
	bk_err_t r1 = bk_i2c_master_write(I2C_LB_MST_ID, 0x7E, d, sizeof(d), 5);
	if (r1 == BK_OK) {
		errs++;
		I2C_TC_FAIL("recover.short_timeout_recover", "step1 unexpected success on 5ms timeout");
	} else {
		I2C_TC_PASS("recover.short_timeout_recover", "step1 short-timeout err=%d as expected", r1);
	}
	uint32_t mm = 0;
	bk_err_t r2 = i2c_lb_run_once(len, I2C_LB_SLV_READ, &mm);
	errs += i2c_lb_eval("recover.short_timeout_recover.step2", r2, mm, len);
	return errs;
}

static int i2c_rc_slave_fifo_resync(uint32_t len)
{
	int errs = 0;
	uint32_t mm = 0;
	bk_err_t r1 = i2c_lb_run_once(len, I2C_LB_SLV_READ, &mm);
	errs += i2c_lb_eval("recover.slave_fifo_resync.pre", r1, mm, len);

	/* reset the slave controller mid-stream to force FIFO/state resync */
	bk_i2c_deinit(I2C_LB_SLV_ID);
	i2c_config_t scfg = {0};
	scfg.baud_rate = I2C_LB_BAUD;
	scfg.addr_mode = I2C_ADDR_MODE_7BIT;
	scfg.slave_addr = I2C_LB_SLV_ADDR;
	if (bk_i2c_init(I2C_LB_SLV_ID, &scfg) != BK_OK) {
		errs++;
		I2C_TC_FAIL("recover.slave_fifo_resync", "slave re-init failed");
		return errs;
	}
	mm = 0;
	bk_err_t r2 = i2c_lb_run_once(len, I2C_LB_SLV_READ, &mm);
	errs += i2c_lb_eval("recover.slave_fifo_resync.post", r2, mm, len);
	return errs;
}

#if I2C_LB_STUCK_BUS_EN
static int i2c_rc_stuck_bus(uint32_t len)
{
	int errs = 0;
	/* Hold SDA low via a spare GPIO physically wired to the bus -> bus stuck busy */
	bk_gpio_enable_output(I2C_LB_STUCK_GPIO);
	bk_gpio_set_output_low(I2C_LB_STUCK_GPIO);
	rtos_delay_milliseconds(2);

	uint8_t d[2] = {0x55, 0xAA};
	bk_err_t r1 = bk_i2c_master_write(I2C_LB_MST_ID, I2C_LB_SLV_ADDR, d, sizeof(d), 50);
	if (r1 != BK_OK) {
		I2C_TC_PASS("recover.stuck_bus", "step1 bus-busy/timeout detected err=%d", r1);
	} else {
		errs++;
		I2C_TC_FAIL("recover.stuck_bus", "step1 expected bus-busy, got BK_OK");
	}

	/* release line, reset controllers, retry a real transfer */
	bk_gpio_set_output_high(I2C_LB_STUCK_GPIO);
	bk_gpio_enable_input(I2C_LB_STUCK_GPIO);
	rtos_delay_milliseconds(2);
	i2c_lb_teardown();
	if (i2c_lb_setup() != BK_OK) {
		errs++;
		I2C_TC_FAIL("recover.stuck_bus", "re-setup failed");
		return errs;
	}
	uint32_t mm = 0;
	bk_err_t r2 = i2c_lb_run_once(len, I2C_LB_SLV_READ, &mm);
	errs += i2c_lb_eval("recover.stuck_bus.step2", r2, mm, len);
	return errs;
}
#endif

static void cli_i2c_help(void)
{
	CLI_LOGD("i2c_driver init\r\n");
	CLI_LOGD("i2c_driver deinit\r\n");
	CLI_LOGD("i2c {id} init baudrate\r\n");
	CLI_LOGD("i2c {id} deinit\r\n");
	CLI_LOGD("i2c {id} memory_write {data_size}\r\n");
	CLI_LOGD("i2c {id} memory_read {data_size}\r\n");
	CLI_LOGD("i2c {id} zd24c128_write <mem_addr_hex> <len>  - ZD24C128 write (addr 16bit)\r\n");
	CLI_LOGD("i2c {id} zd24c128_read <mem_addr_hex> <len>   - ZD24C128 read\r\n");
	CLI_LOGD("i2c {id} zd24c128_verify <mem_addr_hex> <len> - ZD24C128 write then read verify\r\n");
	CLI_LOGD("i2c {id} tc_nack [dev_addr_hex]               - expect NACK/timeout from absent device\r\n");
	CLI_LOGD("i2c {id} tc_invalid_param                     - NULL cfg / un-init id error-code checks\r\n");
	CLI_LOGD("i2c {id} tc_mem_leak [loops] [tol_bytes]      - init/deinit heap leak check\r\n");
	CLI_LOGD("--- single-board co-test: jumper I2C0.SDA<->I2C1.SDA, I2C0.SCL<->I2C1.SCL + pull-ups ---\r\n");
	CLI_LOGD("i2c {id} loopback {mst_wr_slv_rd|mst_rd_slv_wr|repeat|all} [len] [loops] - I2C0<->I2C1 auto co-test\r\n");
	CLI_LOGD("i2c {id} recover {nack_then_ok|short_timeout_recover|slave_fifo_resync|stuck_bus|all} [len] - exception recovery\r\n");
}

static void cli_i2c_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_i2c_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_i2c_driver_init());
		CLI_LOGD("i2c driver init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_i2c_driver_deinit());
		CLI_LOGD("i2c driver deinit\n");
	} else {
		cli_i2c_help();
		return;
	}
}

static void cli_i2c_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t data_length = 64;

	if (argc < 2) {
		cli_i2c_help();
		return;
	}

	uint32_t i2c_id = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "init") == 0) {
		i2c_config_t i2c_cfg = {0};
		i2c_cfg.baud_rate = 400000;

		if(argc > 3){
				i2c_cfg.baud_rate = os_strtoul(argv[3], NULL, 10);
		}

		if(argc > 4){
			i2c_cfg.src_clk = os_strtoul(argv[4], NULL, 10);
			CLI_LOGI("i2c(%d) init, src_clk=%d\n", i2c_id, i2c_cfg.src_clk);
		}

		i2c_cfg.addr_mode = I2C_ADDR_MODE_7BIT;
		i2c_cfg.slave_addr = I2C_SLAVE_ADDR;

		CLI_LOGD("i2c[%d]_config rate:0x%x\n", i2c_id, i2c_cfg.baud_rate);

		BK_LOG_ON_ERR(bk_i2c_init(i2c_id, &i2c_cfg));
		CLI_LOGD("bk_i2c[%d]_init success\n", i2c_id);
	} else if (os_strcmp(argv[2], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_i2c_deinit(i2c_id));
		CLI_LOGD("i2c(%d) deinit success\n", i2c_id);
	} else if (os_strcmp(argv[2], "master_write") == 0) {
		uint8_t *data_buf;
		BUF_CTOR();

		for (uint32_t i = 0; i < data_length; i++) {
			data_buf[i] = i & 0xff;
		}
		BK_LOG_ON_ERR(bk_i2c_master_write(i2c_id, I2C_SLAVE_ADDR, data_buf, data_length, I2C_WRITE_WAIT_MAX_MS));
		BUF_DTOR();
	} else if (os_strcmp(argv[2], "master_read") == 0) {
		uint8_t *data_buf;
		BUF_CTOR();

		BK_LOG_ON_ERR(bk_i2c_master_read(i2c_id, I2C_SLAVE_ADDR, data_buf, data_length, I2C_READ_WAIT_MAX_MS));
		for (uint32_t i = 0; i < data_length; i++) {
			CLI_LOGD("cli_test i2c_master read 0x%x,\n", data_buf[i]);
		}
		BUF_DTOR();
#ifndef CONFIG_SIM_I2C
	} else if (os_strcmp(argv[2], "slave_write") == 0) {
		uint8_t *data_buf;
		BUF_CTOR();

		for (uint32_t i = 0; i < data_length; i++) {
			data_buf[i] = i & 0xff;
		}
		BK_LOG_ON_ERR(bk_i2c_slave_write(i2c_id, data_buf, data_length, BEKEN_NEVER_TIMEOUT));
                BUF_DTOR();
	} else if (os_strcmp(argv[2], "slave_read") == 0) {
		uint8_t *data_buf;
		BUF_CTOR();

		BK_LOG_ON_ERR(bk_i2c_slave_read(i2c_id, data_buf, data_length, BEKEN_NEVER_TIMEOUT));
		for (uint32_t i = 0; i < data_length; i++) {
			CLI_LOGD("cli_test i2c_slave read 0x%x,\n", data_buf[i]);
		}
		BUF_DTOR();
	} else if (os_strcmp(argv[2], "set_slave_addr") == 0) {
		uint32_t slave_addr = os_strtoul(argv[3], NULL, 16);
		bk_i2c_set_slave_address(i2c_id, slave_addr);
		CLI_LOGD("i2c_slave set address 0x%x.\n", slave_addr);
#endif
	} else if (os_strcmp(argv[2], "memory_write") == 0) {
		/* Write a known ramp pattern then read it back and auto-verify. */
		uint8_t *data_buf;
		BUF_CTOR();

		for (uint32_t i = 0; i < data_length; i++) {
			data_buf[i] = (i + 1) & 0xff;
		}
		uint32_t dev_addr = (argc > 4) ? os_strtoul(argv[4], NULL, 16) : EEPROM_DEV_ADDR;
		i2c_mem_param_t mem_param = {0};
		mem_param.dev_addr = dev_addr;
		mem_param.mem_addr = EEPROM_MEM_ADDR;
		mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
		mem_param.data = data_buf;
		mem_param.data_size = data_length;
		mem_param.timeout_ms = I2C_WRITE_WAIT_MAX_MS;
		bk_err_t wret = bk_i2c_memory_write(i2c_id, &mem_param);
		if (wret != BK_OK) {
			I2C_TC_FAIL("memory_write", "write ret=%d (dev=0x%x len=%u)", wret, (unsigned)dev_addr, (unsigned)data_length);
			BUF_DTOR();
			return;
		}
		rtos_delay_milliseconds(ZD24C128_WRITE_CYCLE_MS);
		bk_err_t rret = BK_OK;
		uint32_t errs = i2c_mem_readback_cmp(i2c_id, dev_addr, EEPROM_MEM_ADDR, I2C_MEM_ADDR_SIZE_8BIT, data_buf, data_length, &rret);
		if (rret != BK_OK)
			I2C_TC_FAIL("memory_write", "readback ret=%d (dev=0x%x len=%u)", rret, (unsigned)dev_addr, (unsigned)data_length);
		else if (errs)
			I2C_TC_FAIL("memory_write", "%u/%u bytes mismatch (dev=0x%x)", (unsigned)errs, (unsigned)data_length, (unsigned)dev_addr);
		else
			I2C_TC_PASS("memory_write", "dev=0x%x len=%u write+readback verified", (unsigned)dev_addr, (unsigned)data_length);
		BUF_DTOR();
	} else if (os_strcmp(argv[2], "memory_read") == 0) {
		/* Read back and auto-compare against the ramp pattern memory_write stores. */
		uint8_t *data_buf;
		BUF_CTOR();

		i2c_mem_param_t mem_param = {0};
		mem_param.dev_addr = EEPROM_DEV_ADDR;
		mem_param.mem_addr = EEPROM_MEM_ADDR;
		mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
		mem_param.data = data_buf;
		mem_param.data_size = data_length;
		mem_param.timeout_ms = I2C_WRITE_WAIT_MAX_MS;

		bk_err_t rret = bk_i2c_memory_read(i2c_id, &mem_param);
		if (rret != BK_OK) {
			I2C_TC_FAIL("memory_read", "read ret=%d (len=%u)", rret, (unsigned)data_length);
			BUF_DTOR();
			return;
		}
		uint32_t errs = 0;
		for (uint32_t i = 0; i < data_length; i++) {
			uint8_t expect = (i + 1) & 0xff;
			CLI_LOGD("i2c_read_buf[%d]=%x\r\n", i, data_buf[i]);
			if (data_buf[i] != expect) {
				if (errs < 8)
					CLI_LOGE("  mismatch [%u]: expect=0x%02x read=0x%02x\r\n", (unsigned)i, expect, data_buf[i]);
				errs++;
			}
		}
		if (errs)
			I2C_TC_FAIL("memory_read", "%u/%u bytes mismatch vs written ramp pattern", (unsigned)errs, (unsigned)data_length);
		else
			I2C_TC_PASS("memory_read", "len=%u match written ramp pattern", (unsigned)data_length);
		BUF_DTOR();
	} else if (os_strcmp(argv[2], "cam_write") == 0) {
		uint32_t buf_len = 1;
		uint8_t data = os_strtoul(argv[3], NULL, 16);
		uint32_t dev_addr = os_strtoul(argv[4], NULL, 16);
		uint32_t mem_addr = os_strtoul(argv[5], NULL, 16);
		i2c_mem_param_t mem_param = {0};
		mem_param.dev_addr = dev_addr;
		mem_param.mem_addr = mem_addr;
		mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
		mem_param.data = &data;
		mem_param.data_size = buf_len;
		mem_param.timeout_ms = I2C_WRITE_WAIT_MAX_MS;
		BK_LOG_ON_ERR(bk_i2c_memory_write(i2c_id, &mem_param));
		CLI_LOGD("i2c(%d) cam_write buf_len:%d\r\n", i2c_id, buf_len);
	} else if (os_strcmp(argv[2], "cam_read") == 0) {
		uint32_t buf_len = 1;
		uint8_t data = os_strtoul(argv[3], NULL, 16);
		uint32_t dev_addr = os_strtoul(argv[4], NULL, 16);
		uint32_t mem_addr = os_strtoul(argv[5], NULL, 16);
		i2c_mem_param_t mem_param = {0};
		mem_param.dev_addr = dev_addr;
		mem_param.mem_addr = mem_addr;
		mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
		mem_param.data = &data;
		mem_param.data_size = buf_len;
		mem_param.timeout_ms = I2C_WRITE_WAIT_MAX_MS;
		BK_LOG_ON_ERR(bk_i2c_memory_read(i2c_id, &mem_param));
		CLI_LOGD("i2c_read_buf = %x\r\n", data);
	    CLI_LOGD("i2c_read_buf = %x\r\n", *(mem_param.data));
		CLI_LOGD("i2c(%d) cam_read buf_len:%d\r\n", i2c_id, buf_len);
	} else if (os_strcmp(argv[2], "cam_test1") == 0) {
		uint32_t buf_len = 1;
		uint32_t cmp_len = sizeof(sensor_gc0328c_init_talbe_test1)/2;
		uint8_t *data_buf = os_zalloc(cmp_len);
		if(NULL == data_buf)
		{
			CLI_LOGE("os_zalloc fail\r\n");
			return;
		}
		BK_LOGD(NULL, "sizeof(sensor_gc0328c_init_talbe_test1) = %d\n ", sizeof(sensor_gc0328c_init_talbe_test1));
		uint32_t err_cnt = 0;
		for(int i = 0; i < cmp_len; i++) {
			uint32_t dev_addr = CAMERA_DEV_ADDR;
			uint32_t mem_addr = sensor_gc0328c_init_talbe_test1[i][0];

			i2c_mem_param_t mem_param_w = {0};
			mem_param_w.dev_addr = dev_addr;
			mem_param_w.mem_addr = mem_addr;
			mem_param_w.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
			mem_param_w.data = &(sensor_gc0328c_init_talbe_test1[i][1]);
			mem_param_w.data_size = buf_len;
			mem_param_w.timeout_ms = I2C_WRITE_WAIT_MAX_MS;
			BK_LOG_ON_ERR(bk_i2c_memory_write(i2c_id, &mem_param_w));

			i2c_mem_param_t mem_param = {0};
			mem_param.dev_addr = dev_addr;
			mem_param.mem_addr = mem_addr;
			mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
			mem_param.data = data_buf + i;
			mem_param.data_size = buf_len;
			mem_param.timeout_ms = I2C_WRITE_WAIT_MAX_MS;
			BK_LOG_ON_ERR(bk_i2c_memory_read(i2c_id, &mem_param));
			CLI_LOGD("i2c(%d) cam_read addr:0x%x, data:0x%x\r\n", i2c_id, sensor_gc0328c_init_talbe_test1[i][0], data_buf[i]);
			if(sensor_gc0328c_init_talbe_test1[i][1] != data_buf[i]) {
				CLI_LOGE("i2c(%d) cam_read addr:0x%x, data=0x%x,  correct_data=0x%x.\r\n", i2c_id, sensor_gc0328c_init_talbe_test1[i][0], data_buf[i], sensor_gc0328c_init_talbe_test1[i][1]);
				gpio_debug(19);
				err_cnt++;
			}
		}
		if (err_cnt)
			I2C_TC_FAIL("cam_test1", "%u/%u regs mismatch", (unsigned)err_cnt, (unsigned)cmp_len);
		else
			I2C_TC_PASS("cam_test1", "%u regs write+readback verified", (unsigned)cmp_len);
		if (data_buf) {
			os_free(data_buf);
			data_buf = NULL;
		}
	} else if (os_strcmp(argv[2], "cam_test") == 0) {
		uint32_t buf_len = 1;
		uint32_t cmp_len = 2048;
		uint8_t *data_buf = os_zalloc(cmp_len);
		if(NULL == data_buf)
		{
			CLI_LOGE("os_zalloc fail\r\n");
			return;
		}
		uint32_t err_cnt = 0;
		for(int i = 0; i < cmp_len; i++) {
			uint32_t dev_addr = CAMERA_DEV_ADDR;
			uint32_t mem_addr = 0x84;
			uint8_t test_data = i & 0xff;

			i2c_mem_param_t mem_param_w = {0};
			mem_param_w.dev_addr = dev_addr;
			mem_param_w.mem_addr = mem_addr;
			mem_param_w.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
			mem_param_w.data = &(test_data);
			mem_param_w.data_size = buf_len;
			mem_param_w.timeout_ms = I2C_WRITE_WAIT_MAX_MS;
			BK_LOG_ON_ERR(bk_i2c_memory_write(i2c_id, &mem_param_w));

			i2c_mem_param_t mem_param = {0};
			mem_param.dev_addr = dev_addr;
			mem_param.mem_addr = mem_addr;
			mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
			mem_param.data = data_buf + i;
			mem_param.data_size = buf_len;
			mem_param.timeout_ms = I2C_WRITE_WAIT_MAX_MS;
			BK_LOG_ON_ERR(bk_i2c_memory_read(i2c_id, &mem_param));
			CLI_LOGD("i2c(%d) cam_read addr:0x%x, data:0x%x\r\n", i2c_id, mem_addr, data_buf[i]);
			if(test_data != data_buf[i]) {
				CLI_LOGE("i2c(%d) cam_read addr:0x%x, data=0x%x,  correct_data=0x%x.\r\n", i2c_id, mem_addr, data_buf[i], test_data);
				gpio_debug(18);
				err_cnt++;
			}
		}
		if (err_cnt)
			I2C_TC_FAIL("cam_test", "%u/%u iterations mismatch", (unsigned)err_cnt, (unsigned)cmp_len);
		else
			I2C_TC_PASS("cam_test", "%u write+readback iterations verified", (unsigned)cmp_len);
		if (data_buf) {
			os_free(data_buf);
			data_buf = NULL;
		}
	} else if (os_strcmp(argv[2], "cam_dump") == 0) {
		uint32_t buf_len = 1;
		uint32_t cmp_len = 256;
		uint8_t *data_buf = os_zalloc(cmp_len);
		if(NULL == data_buf)
		{
			CLI_LOGE("os_zalloc fail\r\n");
			return;
		}
		uint32_t dev_addr = CAMERA_DEV_ADDR;
		uint32_t mem_addr = 0;
		i2c_mem_param_t mem_param = {0};

		for(int i = 0; i < cmp_len; i++) {
			mem_param.dev_addr = dev_addr;
			mem_param.mem_addr = mem_addr;
			mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
			mem_param.data = data_buf + i;
			mem_param.data_size = buf_len;
			mem_param.timeout_ms = I2C_WRITE_WAIT_MAX_MS;
			BK_LOG_ON_ERR(bk_i2c_memory_read(i2c_id, &mem_param));
			mem_addr++;
		}
		BK_LOGD(NULL, "\r\n");
		for (uint32_t i = 0; i < 16; i++) {
			BK_LOGD(NULL, "REG%01x0 : ", i);
			for (uint32_t j = 0; j < 16; j++) {
				BK_LOGD(NULL, "%02x ", data_buf[i * 16 + j]);
			}
			BK_LOGD(NULL, "\r\n");
		}
		if (data_buf) {
			os_free(data_buf);
			data_buf = NULL;
		}
	} else if (os_strcmp(argv[2], "zd24c128_write") == 0) {
		/* i2c {id} zd24c128_write <mem_addr_hex> <len> */
		CLI_RET_ON_INVALID_ARGC(argc, 5);
		uint32_t mem_addr = os_strtoul(argv[3], NULL, 16);
		uint32_t len = os_strtoul(argv[4], NULL, 10);
		if (len == 0 || mem_addr + len > ZD24C128_SIZE) {
			CLI_LOGE("zd24c128: invalid addr 0x%x or len %u (max size %u)\r\n", (unsigned)mem_addr, (unsigned)len, (unsigned)ZD24C128_SIZE);
			return;
		}
		uint8_t *data_buf = os_malloc(len);
		if (!data_buf) {
			CLI_LOGE("zd24c128_write malloc fail\r\n");
			return;
		}
		for (uint32_t i = 0; i < len; i++)
			data_buf[i] = (uint8_t)((i + 1) & 0xff);
		uint32_t off = 0;
		while (off < len) {
			uint32_t chunk = len - off;
			if (chunk > ZD24C128_PAGE_SIZE)
				chunk = ZD24C128_PAGE_SIZE;
			i2c_mem_param_t mem_param = {0};
			mem_param.dev_addr = ZD24C128_DEV_ADDR;
			mem_param.mem_addr = mem_addr + off;
			mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
			mem_param.data = data_buf + off;
			mem_param.data_size = chunk;
			mem_param.timeout_ms = I2C_WRITE_WAIT_MAX_MS;
			bk_err_t ret = bk_i2c_memory_write(i2c_id, &mem_param);
			if (ret != BK_OK) {
				CLI_LOGE("zd24c128_write fail at off %u\r\n", (unsigned)off);
				os_free(data_buf);
				return;
			}
			off += chunk;
			rtos_delay_milliseconds(ZD24C128_WRITE_CYCLE_MS);
		}
		os_free(data_buf);
		CLI_LOGI("zd24c128_write ok: addr=0x%x len=%u\r\n", (unsigned)mem_addr, (unsigned)len);
	} else if (os_strcmp(argv[2], "zd24c128_read") == 0) {
		/* i2c {id} zd24c128_read <mem_addr_hex> <len> */
		CLI_RET_ON_INVALID_ARGC(argc, 5);
		uint32_t mem_addr = os_strtoul(argv[3], NULL, 16);
		uint32_t len = os_strtoul(argv[4], NULL, 10);
		if (len == 0 || mem_addr + len > ZD24C128_SIZE) {
			CLI_LOGE("zd24c128: invalid addr 0x%x or len %u (max size %u)\r\n", (unsigned)mem_addr, (unsigned)len, (unsigned)ZD24C128_SIZE);
			return;
		}
		uint8_t *data_buf = os_zalloc(len);
		if (!data_buf) {
			CLI_LOGE("zd24c128_read malloc fail\r\n");
			return;
		}
		i2c_mem_param_t mem_param = {0};
		mem_param.dev_addr = ZD24C128_DEV_ADDR;
		mem_param.mem_addr = mem_addr;
		mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
		mem_param.data = data_buf;
		mem_param.data_size = len;
		mem_param.timeout_ms = I2C_READ_WAIT_MAX_MS;
		bk_err_t ret = bk_i2c_memory_read(i2c_id, &mem_param);
		if (ret != BK_OK) {
			CLI_LOGE("zd24c128_read fail\r\n");
			os_free(data_buf);
			return;
		}
		CLI_LOGI("zd24c128_read addr=0x%x len=%u:\r\n", (unsigned)mem_addr, (unsigned)len);
		for (uint32_t i = 0; i < len; i++) {
			CLI_LOGD("%02x ", data_buf[i]);
			if ((i + 1) % 16 == 0)
				CLI_LOGD("\r\n");
		}
		if (len % 16 != 0)
			CLI_LOGD("\r\n");
		os_free(data_buf);
	} else if (os_strcmp(argv[2], "zd24c128_verify") == 0) {
		/* i2c {id} zd24c128_verify <mem_addr_hex> <len> */
		CLI_RET_ON_INVALID_ARGC(argc, 5);
		uint32_t mem_addr = os_strtoul(argv[3], NULL, 16);
		uint32_t len = os_strtoul(argv[4], NULL, 10);
		if (len == 0 || mem_addr + len > ZD24C128_SIZE) {
			CLI_LOGE("zd24c128: invalid addr 0x%x or len %u (max size %u)\r\n", (unsigned)mem_addr, (unsigned)len, (unsigned)ZD24C128_SIZE);
			return;
		}
		uint8_t *wr_buf = os_malloc(len);
		uint8_t *rd_buf = os_zalloc(len);
		if (!wr_buf || !rd_buf) {
			CLI_LOGE("zd24c128_verify malloc fail\r\n");
			if (wr_buf) os_free(wr_buf);
			if (rd_buf) os_free(rd_buf);
			return;
		}
		for (uint32_t i = 0; i < len; i++)
			wr_buf[i] = (uint8_t)((i + 1) & 0xff);
		uint32_t off = 0;
		while (off < len) {
			uint32_t chunk = len - off;
			if (chunk > ZD24C128_PAGE_SIZE)
				chunk = ZD24C128_PAGE_SIZE;
			i2c_mem_param_t mem_param = {0};
			mem_param.dev_addr = ZD24C128_DEV_ADDR;
			mem_param.mem_addr = mem_addr + off;
			mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
			mem_param.data = wr_buf + off;
			mem_param.data_size = chunk;
			mem_param.timeout_ms = I2C_WRITE_WAIT_MAX_MS;
			if (bk_i2c_memory_write(i2c_id, &mem_param) != BK_OK) {
				CLI_LOGE("zd24c128_verify write fail at off %u\r\n", (unsigned)off);
				os_free(wr_buf);
				os_free(rd_buf);
				return;
			}
			off += chunk;
			rtos_delay_milliseconds(ZD24C128_WRITE_CYCLE_MS);
		}
		rtos_delay_milliseconds(ZD24C128_WRITE_CYCLE_MS);
		i2c_mem_param_t rparam = {0};
		rparam.dev_addr = ZD24C128_DEV_ADDR;
		rparam.mem_addr = mem_addr;
		rparam.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
		rparam.data = rd_buf;
		rparam.data_size = len;
		rparam.timeout_ms = I2C_READ_WAIT_MAX_MS;
		if (bk_i2c_memory_read(i2c_id, &rparam) != BK_OK) {
			CLI_LOGE("zd24c128_verify read fail\r\n");
			os_free(wr_buf);
			os_free(rd_buf);
			return;
		}
		uint32_t err_cnt = 0;
		for (uint32_t i = 0; i < len; i++) {
			if (rd_buf[i] != wr_buf[i]) {
				if (err_cnt < 8)
					CLI_LOGE("mismatch at [%u]: wr=0x%02x rd=0x%02x\r\n", (unsigned)i, wr_buf[i], rd_buf[i]);
				err_cnt++;
			}
		}
		os_free(wr_buf);
		os_free(rd_buf);
		if (err_cnt == 0)
			CLI_LOGI("zd24c128_verify PASS: addr=0x%x len=%u\r\n", (unsigned)mem_addr, (unsigned)len);
		else
			CLI_LOGE("zd24c128_verify FAIL: %u errors\r\n", (unsigned)err_cnt);
	} else if (os_strcmp(argv[2], "tc_nack") == 0) {
		/* Negative: write to an absent device address, expect NACK/ACK-timeout, not BK_OK.
		 * i2c {id} tc_nack [dev_addr_hex]  (default 0x7E, a reserved/normally-absent address) */
		uint32_t dev_addr = (argc > 3) ? os_strtoul(argv[3], NULL, 16) : 0x7E;
		BK_LOG_ON_ERR(bk_i2c_driver_init());
		i2c_config_t cfg = {0};
		cfg.baud_rate = 400000;
		cfg.addr_mode = I2C_ADDR_MODE_7BIT;
		cfg.slave_addr = I2C_SLAVE_ADDR;
		if (bk_i2c_init(i2c_id, &cfg) != BK_OK) {
			I2C_TC_FAIL("tc_nack", "i2c(%d) init failed", i2c_id);
			return;
		}
		uint8_t d[2] = {0xA5, 0x5A};
		bk_err_t ret = bk_i2c_master_write(i2c_id, dev_addr, d, sizeof(d), 100);
		if (ret == BK_OK)
			I2C_TC_FAIL("tc_nack", "unexpected ACK from absent dev 0x%x (ret=BK_OK)", (unsigned)dev_addr);
		else
			I2C_TC_PASS("tc_nack", "absent dev 0x%x returned err=%d as expected", (unsigned)dev_addr, ret);
		bk_i2c_deinit(i2c_id);
	} else if (os_strcmp(argv[2], "tc_invalid_param") == 0) {
		/* Negative: verify defensive error codes for bad parameters / wrong state. */
		uint32_t errs = 0;
		BK_LOG_ON_ERR(bk_i2c_driver_init());

		bk_err_t r1 = bk_i2c_init(i2c_id, NULL);
		if (r1 == BK_ERR_NULL_PARAM) {
			I2C_TC_PASS("tc_invalid_param", "init(NULL cfg) -> BK_ERR_NULL_PARAM");
		} else {
			errs++;
			I2C_TC_FAIL("tc_invalid_param", "init(NULL cfg) ret=%d, expect BK_ERR_NULL_PARAM(%d)", r1, BK_ERR_NULL_PARAM);
		}

		bk_i2c_deinit(i2c_id); /* make sure this id is de-initialized */
		uint8_t d = 0;
		bk_err_t r2 = bk_i2c_master_write(i2c_id, I2C_SLAVE_ADDR, &d, 1, 100);
		if (r2 == BK_ERR_I2C_ID_NOT_INIT) {
			I2C_TC_PASS("tc_invalid_param", "master_write on un-init id -> BK_ERR_I2C_ID_NOT_INIT");
		} else {
			errs++;
			I2C_TC_FAIL("tc_invalid_param", "master_write un-init id ret=%d, expect BK_ERR_I2C_ID_NOT_INIT(%d)", r2, BK_ERR_I2C_ID_NOT_INIT);
		}

		bk_err_t r3 = bk_i2c_memory_write(i2c_id, NULL);
		if (r3 == BK_ERR_NULL_PARAM) {
			I2C_TC_PASS("tc_invalid_param", "memory_write(NULL mem_param) -> BK_ERR_NULL_PARAM");
		} else {
			errs++;
			I2C_TC_FAIL("tc_invalid_param", "memory_write(NULL) ret=%d, expect BK_ERR_NULL_PARAM(%d)", r3, BK_ERR_NULL_PARAM);
		}

		bk_err_t r4 = bk_i2c_memory_read(i2c_id, NULL);
		if (r4 == BK_ERR_NULL_PARAM) {
			I2C_TC_PASS("tc_invalid_param", "memory_read(NULL mem_param) -> BK_ERR_NULL_PARAM");
		} else {
			errs++;
			I2C_TC_FAIL("tc_invalid_param", "memory_read(NULL) ret=%d, expect BK_ERR_NULL_PARAM(%d)", r4, BK_ERR_NULL_PARAM);
		}

		if (errs == 0)
			I2C_TC_PASS("tc_invalid_param", "all negative-param sub-checks passed");
		else
			I2C_TC_FAIL("tc_invalid_param", "%u sub-check(s) failed", (unsigned)errs);
	} else if (os_strcmp(argv[2], "tc_mem_leak") == 0) {
		/* Resource: repeated init/deinit must not leak heap.
		 * i2c {id} tc_mem_leak [loops] [tol_bytes]  (defaults: 20 loops, 64 bytes) */
		uint32_t loops = (argc > 3) ? os_strtoul(argv[3], NULL, 10) : 20;
		uint32_t tol   = (argc > 4) ? os_strtoul(argv[4], NULL, 10) : 64;
		if (loops == 0)
			loops = 1;
		BK_LOG_ON_ERR(bk_i2c_driver_init());
		i2c_config_t cfg = {0};
		cfg.baud_rate = 400000;
		cfg.addr_mode = I2C_ADDR_MODE_7BIT;
		cfg.slave_addr = I2C_SLAVE_ADDR;

		/* warm-up once so one-time allocations are excluded from the measurement */
		bk_i2c_init(i2c_id, &cfg);
		bk_i2c_deinit(i2c_id);

		size_t heap_before = rtos_get_free_heap_size();
		for (uint32_t i = 0; i < loops; i++) {
			if (bk_i2c_init(i2c_id, &cfg) != BK_OK) {
				I2C_TC_FAIL("tc_mem_leak", "init failed at loop %u", (unsigned)i);
				return;
			}
			bk_i2c_deinit(i2c_id);
		}
		size_t heap_after = rtos_get_free_heap_size();
		long drift = (long)heap_before - (long)heap_after;
		long adrift = (drift < 0) ? -drift : drift;
		if ((uint32_t)adrift <= tol)
			I2C_TC_PASS("tc_mem_leak", "%u loops, heap drift=%ld bytes (<= %u)", (unsigned)loops, drift, (unsigned)tol);
		else
			I2C_TC_FAIL("tc_mem_leak", "%u loops, heap drift=%ld bytes (> %u) LEAK", (unsigned)loops, drift, (unsigned)tol);
	} else if (os_strcmp(argv[2], "loopback") == 0) {
		/* Single-board I2C0<->I2C1 co-test (id arg is a placeholder, ids fixed by macro).
		 * i2c {id} loopback {mst_wr_slv_rd|mst_rd_slv_wr|repeat|all} [len] [loops] */
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		const char *sub = argv[3];
		uint32_t len = (argc > 4) ? os_strtoul(argv[4], NULL, 10) : I2C_LB_DEF_LEN;
		if (len == 0)
			len = I2C_LB_DEF_LEN;
		if (i2c_lb_setup() != BK_OK) {
			I2C_TC_FAIL("loopback", "setup failed (driver/controller init)");
			i2c_lb_teardown();
			return;
		}
		if (os_strcmp(sub, "mst_wr_slv_rd") == 0) {
			uint32_t mm = 0;
			bk_err_t r = i2c_lb_run_once(len, I2C_LB_SLV_READ, &mm);
			i2c_lb_eval("loopback.mst_wr_slv_rd", r, mm, len);
		} else if (os_strcmp(sub, "mst_rd_slv_wr") == 0) {
			uint32_t mm = 0;
			bk_err_t r = i2c_lb_run_once(len, I2C_LB_SLV_WRITE, &mm);
			i2c_lb_eval("loopback.mst_rd_slv_wr", r, mm, len);
		} else if (os_strcmp(sub, "repeat") == 0) {
			uint32_t loops = (argc > 5) ? os_strtoul(argv[5], NULL, 10) : 20;
			if (loops == 0)
				loops = 1;
			uint32_t fails = 0;
			for (uint32_t i = 0; i < loops; i++) {
				uint32_t mm = 0;
				int mode = (i & 1) ? I2C_LB_SLV_WRITE : I2C_LB_SLV_READ;
				bk_err_t r = i2c_lb_run_once(len, mode, &mm);
				if (r != BK_OK || mm) {
					fails++;
					CLI_LOGE("  iter %u fail: ret=%d mismatch=%u\r\n", (unsigned)i, r, (unsigned)mm);
				}
			}
			if (fails == 0)
				I2C_TC_PASS("loopback.repeat", "%u iters len=%u all verified", (unsigned)loops, (unsigned)len);
			else
				I2C_TC_FAIL("loopback.repeat", "%u/%u iters failed", (unsigned)fails, (unsigned)loops);
		} else if (os_strcmp(sub, "all") == 0) {
			uint32_t fails = 0, mm = 0;
			bk_err_t r;
			r = i2c_lb_run_once(len, I2C_LB_SLV_READ, &mm);
			fails += i2c_lb_eval("loopback.mst_wr_slv_rd", r, mm, len);
			r = i2c_lb_run_once(len, I2C_LB_SLV_WRITE, &mm);
			fails += i2c_lb_eval("loopback.mst_rd_slv_wr", r, mm, len);
			if (fails == 0)
				I2C_TC_PASS("loopback.all", "all sub-cases passed");
			else
				I2C_TC_FAIL("loopback.all", "%u sub-case(s) failed", (unsigned)fails);
		} else {
			CLI_LOGE("unknown loopback sub: %s\r\n", sub);
		}
		i2c_lb_teardown();
	} else if (os_strcmp(argv[2], "recover") == 0) {
		/* i2c {id} recover {nack_then_ok|short_timeout_recover|slave_fifo_resync|stuck_bus|all} [len] */
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		const char *sub = argv[3];
		uint32_t len = (argc > 4) ? os_strtoul(argv[4], NULL, 10) : I2C_LB_DEF_LEN;
		if (len == 0)
			len = I2C_LB_DEF_LEN;

		if (os_strcmp(sub, "stuck_bus") == 0) {
#if I2C_LB_STUCK_BUS_EN
			if (i2c_lb_setup() != BK_OK) {
				I2C_TC_FAIL("recover.stuck_bus", "setup failed");
				i2c_lb_teardown();
				return;
			}
			i2c_rc_stuck_bus(len);
			i2c_lb_teardown();
#else
			I2C_TC_FAIL("recover.stuck_bus", "disabled: set I2C_LB_STUCK_BUS_EN=1 + I2C_LB_STUCK_GPIO (wire spare GPIO to SDA)");
#endif
			return;
		}

		if (i2c_lb_setup() != BK_OK) {
			I2C_TC_FAIL("recover", "setup failed");
			i2c_lb_teardown();
			return;
		}
		int fails = 0, ran = 0;
		if (os_strcmp(sub, "nack_then_ok") == 0 || os_strcmp(sub, "all") == 0) {
			fails += i2c_rc_nack_then_ok(len);
			ran = 1;
		}
		if (os_strcmp(sub, "short_timeout_recover") == 0 || os_strcmp(sub, "all") == 0) {
			fails += i2c_rc_short_timeout_recover(len);
			ran = 1;
		}
		if (os_strcmp(sub, "slave_fifo_resync") == 0 || os_strcmp(sub, "all") == 0) {
			fails += i2c_rc_slave_fifo_resync(len);
			ran = 1;
		}
		i2c_lb_teardown();
		if (!ran) {
			CLI_LOGE("unknown recover sub: %s\r\n", sub);
			return;
		}
		if (os_strcmp(sub, "all") == 0) {
			if (fails == 0)
				I2C_TC_PASS("recover.all", "all recovery sub-cases passed");
			else
				I2C_TC_FAIL("recover.all", "%d sub-check(s) failed", fails);
		}
	} else if (os_strcmp(argv[2], "clk_select") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		if (os_strcmp(argv[3], "xtal") == 0){
			sys_drv_i2c_select_clock(i2c_id, I2C_SCLK_XTAL);
			CLI_LOGI("i2c(%d) set clk_source xtal\r\n", i2c_id);
		} else if (os_strcmp(argv[3], "120m") == 0){
			sys_drv_i2c_select_clock(i2c_id, I2C_SCLK_120M);
			CLI_LOGI("i2c(%d) set clk_source 120m\r\n", i2c_id);
		} else {
			CLI_LOGI("i2c config set failed,clock_source only support xtal/120m\r\n");
			return;
		}
		CLI_LOGI("i2c(%d) config clk_source succeed\r\n", i2c_id);
	} else {
		cli_i2c_help();
		return;
	}
}

#define I2C_CMD_CNT (sizeof(s_i2c_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_i2c_commands[] = {
	{"i2c_driver", "i2c_driver {init|deinit}", cli_i2c_driver_cmd},
	{"i2c", "i2c {init|write|read}", cli_i2c_cmd},
};

int bk_iic_register_cli_test_feature(void)
{
	return cli_register_module_test_feature(s_i2c_commands, I2C_CMD_CNT);
}

